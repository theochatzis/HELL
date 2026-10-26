#!/usr/bin/env python3
"""Generate env/banner.txt and env/banner_ascii.txt: HELL in big block letters under a field of flames.

    python3 scripts/make_banner.py            # rewrite both files
Edit the tongue placements below (x, height) for a different fire; letters are pixel grids scaled x2.
"""
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))
WIDTH = 70

# ---------------- letters: 7x7 pixel grids, scaled 2x horizontally and 2x vertically (12 rows) ----
H = ["XX...XX", "XX...XX", "XXXXXXX", "XX...XX", "XX...XX", "XX...XX"]
E = ["XXXXXXX", "XX.....", "XXXXXX.", "XX.....", "XX.....", "XXXXXXX"]
L = ["XX.....", "XX.....", "XX.....", "XX.....", "XX.....", "XXXXXXX"]


def letter_mask():
    """Pixel mask of the letters (scaled x2 both ways): list of rows of booleans, plus the shadow row."""
    letters = [H, E, L, L]
    rows = []
    for r in range(6):
        for _ in range(2):
            line = [False]
            for li, let in enumerate(letters):
                for c in let[r]:
                    line += [c == "X"] * 2
                if li < 3:
                    line += [False, False]
            rows.append(line)
    return rows


def render_letters(mask, block="█", shade="▓"):
    rows = ["".join(block if v else " " for v in row).rstrip() for row in mask]
    rows.append("".join(shade if v else " " for v in mask[-1]).rstrip())
    return rows


# ---------------- flames ----------------
def tongue(height, rnd):
    """Lines of one flame tongue from top to bottom, as (dx, text) relative to the tongue axis."""
    if height <= 1:
        return [(0, rnd.choice(["'", "`", "."]))]
    lines = [(0, rnd.choice(["'", "*", ".", "`"]))]
    body = [")\\", "/(", "(\\", ")/", " )", "( "]
    wide = ["( \\", ")\\ ", "/ (", " )\\", "(/(", ")\\)"]
    base = ["((_)", ")\\_)", "(_))", " )\\)", "((_)"]
    for i in range(1, height):
        frac = i / max(height - 1, 1)
        if frac < 0.4:
            lines.append((rnd.choice([-1, 0]), rnd.choice(body)))
        elif frac < 0.75:
            lines.append((rnd.choice([-1, -1, 0]), rnd.choice(wide)))
        else:
            lines.append((-2, rnd.choice(base)))
    lines.append((-2, rnd.choice(base[:2] + base[4:])))
    return lines


def render(mask, flame_rows=12, seed=11):
    """Flames rise only from the solid surfaces of the letters: the tops of the posts and bars, the
    middle bar inside the H, the foot of the L. Nothing floats over empty space."""
    rnd = random.Random(seed)
    width = max(len(r) for r in mask)
    solid = [[(c < len(row) and row[c]) for c in range(width)] for row in mask]
    rows = flame_rows + len(mask)
    canvas = [[" "] * width for _ in range(rows)]
    for r in range(len(mask)):
        for c in range(width):
            if solid[r][c]:
                canvas[flame_rows + r][c] = "#"   # placeholder, replaced by the block character later

    def free_above(r, c):
        """Number of empty cells above canvas row r in column c before a solid cell or the top."""
        n = 0
        rr = r - 1
        while rr >= 0 and canvas[rr][c] != "#":
            n += 1
            rr -= 1
        return n

    # surfaces: contiguous runs of solid cells with an empty cell above (tops of posts and bars,
    # the middle bar inside the H, the feet of the L and the notches of the E)
    cells = {(flame_rows + r, c) for r in range(len(mask)) for c in range(width)
             if solid[r][c] and (r == 0 or not solid[r - 1][c])}
    segments = []
    for (r, c) in sorted(cells):
        if segments and segments[-1][0] == r and segments[-1][2] == c - 1:
            segments[-1][2] = c
        else:
            segments.append([r, c, c])
    for r, c0, c1 in segments:
        top_edge = r == flame_rows
        c = c0 + rnd.randint(0, 1)
        spill = 2 if top_edge else 1
        while c <= c1:
            space = free_above(r, c)
            if space > 0:
                h = rnd.choice([3, 4, 5, 5, 6, 7, 8, 9, 11]) if top_edge else rnd.choice([1, 1, 2, 2, 3])
                h = min(h, space - 1) if space > 1 else 1
                lines = tongue(h, rnd)[-space:]
                top = r - len(lines)
                for i, (dx, text) in enumerate(lines):
                    for j, ch in enumerate(text):
                        xx = c + dx + j
                        if c0 - spill <= xx <= c1 + spill and 0 <= xx < width and ch != " " and canvas[top + i][xx] == " ":
                            canvas[top + i][xx] = ch
            c += rnd.choice([2, 2, 3, 3, 4]) if top_edge else rnd.choice([3, 4, 5])
    surfaces = cells
    # sparks drifting above the fire, only over the letters
    cols = sorted({c for (_, c) in surfaces})
    for _ in range(24):
        rr, cc = rnd.randint(0, 4), rnd.choice(cols) + rnd.randint(-1, 1)
        if 0 <= cc < width and canvas[rr][cc] == " ":
            canvas[rr][cc] = rnd.choice(".'*`.'")
    return ["".join(row).rstrip() for row in canvas]


def main():
    subtitle = "H i g h - E n e r g y   L i k e l i h o o d   L a b o r a t o r y"
    mask = letter_mask()
    art = render(mask)
    shadow = render_letters(mask, "#", "=")[-1]
    for name, block, shade in (("banner.txt", "█", "▓"), ("banner_ascii.txt", "#", "=")):
        lines = [l.replace("#", block) for l in art] + [shadow.replace("=", shade), ""]
        pad = max(0, (len(lines[-3]) - len(subtitle)) // 2)
        lines.append(" " * pad + subtitle)
        with open(os.path.join(HERE, "..", "env", name), "w", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")
    print(open(os.path.join(HERE, "..", "env", "banner.txt"), encoding="utf-8").read())


if __name__ == "__main__":
    main()
