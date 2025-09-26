import pcbnew

X_OFFSET_MM = 7
Y_OFFSET_MM = -1

def mm(val):
    return pcbnew.FromMM(val)

board = pcbnew.GetBoard()
moved_count = 0

for fp in board.GetFootprints():
    lc_ref = fp.GetReference()
    if not lc_ref.startswith("LC"):
        continue

    index_str = lc_ref[2:]
    if not index_str.isdigit():
        print(f"Skipping {lc_ref} (non-numeric index)")
        continue

    index = int(index_str)
    if index == 0:
        print(f"Skipping {lc_ref} (zero index)")
        continue

    led_ref = f"LED{index}"
    led_fp = board.FindFootprintByReference(led_ref)

    if led_fp is None:
        print(f"{led_ref} not found for {lc_ref}")
        continue

    led_pos = led_fp.GetPosition()
    new_pos = pcbnew.VECTOR2I(
        int(led_pos.x + mm(X_OFFSET_MM)),
        int(led_pos.y + mm(Y_OFFSET_MM))
    )
    fp.SetPosition(new_pos)
    print(f"Moved {lc_ref} to offset from {led_ref}")
    moved_count += 1

if moved_count == 0:
    print("No footprints moved. Check your LC/LED reference names.")
else:
    print(f"Done. Moved {moved_count} footprints.")
