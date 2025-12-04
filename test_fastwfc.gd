extends Node2D
## Test script to compare GDScript WFC solver vs FastWFC C++ solver

@onready var sample_map: TileMap = $"/root/TestFastWFC/sample"
@onready var target_map: TileMap = $"/root/TestFastWFC/target"

func _ready():
  # Wait a frame for everything to initialize
  await get_tree().process_frame
  run_fastwfc_test()

func run_fastwfc_test():
  print("=== FastWFC Performance Test ===")
  print("")

  # Check if FastWFC is available
  if not ClassDB.class_exists("FastWFCWrapper"):
    push_error("FastWFC extension not loaded! Make sure the .gdextension is in the project.")
    return

  print("FastWFC extension loaded successfully!")

  # Create mapper and learn rules from sample (same as existing addon)
  var mapper = WFCTileMapMapper2D.new()
  mapper.learn_from(sample_map)

  var rules = WFCRules2D.new()
  rules.mapper = mapper
  rules.probabilities_enabled = false  # Disable probabilities to avoid assertion
  rules.learn_from(sample_map)

  print("Tiles learned: ", mapper.size())
  print("Map size: 121 x 68 (8228 cells)")
  print("")

  # Convert rules to FastWFC format
  var tile_data = create_tile_data(mapper)
  var adjacency_rules = convert_rules_to_fastwfc(rules, mapper)

  print("Adjacency rules count: ", adjacency_rules.size())
  print("")

  # Create FastWFC wrapper and run
  var fast_wfc = ClassDB.instantiate("FastWFCWrapper")
  add_child(fast_wfc)

  var start_time = Time.get_ticks_msec()

  fast_wfc.initialize_tiling(tile_data, adjacency_rules, 121, 68, false, -1)
  var result = fast_wfc.generate()

  var end_time = Time.get_ticks_msec()
  var duration = end_time - start_time

  print("FastWFC C++ Result:")
  print("  Time: %d ms" % duration)
  print("")

  # Verify the result
  var is_valid = verify_result(result, mapper.size(), 121, 68)

  # Render the result to target tilemap
  if is_valid:
    print("")
    print("Rendering result to target TileMap...")
    render_result(result, mapper, target_map)
    print("Done! Check the window to see the generated map below the sample.")

  print("")
  print("Comparison:")
  print("  GDScript solver: ~11000 ms")
  print("  FastWFC C++:     %d ms" % duration)
  if duration > 0:
    print("  Speedup:         %.1fx faster" % (11000.0 / duration))
  print("")
  print("Output valid: ", is_valid)

  fast_wfc.queue_free()

  print("")
  print("Press ESC to quit.")

func _input(event: InputEvent):
  if event.is_action_pressed("ui_cancel"):
    get_tree().quit()

func create_tile_data(mapper: WFCMapper2D) -> Dictionary:
  var tile_data = {}
  for i in range(mapper.size()):
    # FastWFC needs tile content - we'll use simple placeholder
    # The actual content doesn't matter for tiling mode, just the ID
    tile_data[str(i)] = {
      "content": [[i]],  # Simple 1x1 content with tile ID
      "symmetry": "X",   # No rotation/reflection (tiles are pre-oriented)
      "weight": 1.0
    }
  return tile_data

func convert_rules_to_fastwfc(rules: WFCRules2D, mapper: WFCMapper2D) -> Array:
  var adjacency = []
  var num_tiles = mapper.size()

  # axes[0] = (1,0) = right direction
  # axes[1] = (0,1) = down direction

  for axis_idx in range(rules.axis_matrices.size()):
    var matrix: WFCBitMatrix = rules.axis_matrices[axis_idx]

    for tile_a in range(num_tiles):
      for tile_b in range(num_tiles):
        # WFCBitMatrix stores rows as WFCBitSet arrays
        # rows[tile_a].get_bit(tile_b) checks if tile_b is allowed after tile_a
        if matrix.rows[tile_a].get_bit(tile_b):
          # For FastWFC: tile1 is on the left/top, tile2 is on the right/bottom
          adjacency.append({
            "tile1": str(tile_a),
            "orientation1": 0,
            "tile2": str(tile_b),
            "orientation2": 0
          })

  return adjacency

func verify_result(result: Array, num_tiles: int, expected_width: int, expected_height: int) -> bool:
  print("=== Result Verification ===")

  # Check if result is empty
  if result.size() == 0:
    print("ERROR: Result is empty!")
    return false

  # Check dimensions
  var height = result.size()
  var width = result[0].size() if result.size() > 0 else 0
  print("  Dimensions: %d x %d (expected %d x %d)" % [width, height, expected_width, expected_height])

  var dimension_ok = (width == expected_width and height == expected_height)
  if not dimension_ok:
    print("  WARNING: Dimensions don't match expected!")

  # Check for invalid/empty cells and tile ID validity
  var empty_cells = 0
  var invalid_tile_ids = 0
  var total_cells = 0
  var tile_histogram = {}

  for y in range(height):
    var row = result[y]
    if row.size() != width:
      print("  WARNING: Row %d has inconsistent width: %d" % [y, row.size()])

    for x in range(row.size()):
      total_cells += 1
      var cell = row[x]

      # Check for null/empty
      if cell == null or cell == -1:
        empty_cells += 1
        continue

      # Check for valid tile ID range
      if typeof(cell) == TYPE_INT or typeof(cell) == TYPE_FLOAT:
        var tile_id = int(cell)
        if tile_id < 0 or tile_id >= num_tiles:
          invalid_tile_ids += 1
        else:
          tile_histogram[tile_id] = tile_histogram.get(tile_id, 0) + 1
      else:
        # Might be a string tile ID from FastWFC
        var tile_str = str(cell)
        if tile_str.is_valid_int():
          var tile_id = tile_str.to_int()
          if tile_id < 0 or tile_id >= num_tiles:
            invalid_tile_ids += 1
          else:
            tile_histogram[tile_id] = tile_histogram.get(tile_id, 0) + 1
        else:
          invalid_tile_ids += 1

  print("  Total cells: %d" % total_cells)
  print("  Empty/null cells: %d" % empty_cells)
  print("  Invalid tile IDs: %d" % invalid_tile_ids)
  print("  Unique tiles used: %d / %d" % [tile_histogram.size(), num_tiles])

  # Print sample of result (top-left corner)
  print("  Sample (top-left 5x5):")
  for y in range(min(5, height)):
    var row_str = "    "
    for x in range(min(5, result[y].size())):
      row_str += "%3s " % str(result[y][x])
    print(row_str)

  var is_valid = (empty_cells == 0 and invalid_tile_ids == 0 and dimension_ok)
  return is_valid

func render_result(result: Array, mapper: WFCMapper2D, target: TileMap):
  # Clear the target map first
  target.clear()

  # FastWFC result is [y][x] indexed, each cell contains a tile ID
  for y in range(result.size()):
    var row = result[y]
    for x in range(row.size()):
      var cell = row[x]
      var tile_id: int

      # Convert to int if needed
      if typeof(cell) == TYPE_INT:
        tile_id = cell
      elif typeof(cell) == TYPE_FLOAT:
        tile_id = int(cell)
      else:
        tile_id = str(cell).to_int()

      # Write the tile using the mapper
      mapper.write_cell(target, Vector2i(x, y), tile_id)
