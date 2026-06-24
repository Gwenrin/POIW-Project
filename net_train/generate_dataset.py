#!/usr/bin/env python3
"""
Synthetic dataset generator for the OCR net.
Renders each character in multiple fonts and sizes, saves as 28x28 PNG.
Output: dataset/X_NNN.png where X is a safe filename encoding of the character.
"""

from PIL import Image, ImageDraw, ImageFont
import os
import sys
import glob
import random

# Output folder
OUT_DIR = "dataset"
os.makedirs(OUT_DIR, exist_ok=True)

# Characters to generate — must match LABELS in labels.h exactly, in order
CHARS = (
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "ĄĆĘŁŃÓŚŹŻ"
    "ąćęłńóśźż"
    ".,?!\"';:-_()/\\@#$%&*+=<>[]"
)

# Safe filename prefixes — same order as CHARS
# For chars that can't appear in filenames, use a word instead
FILENAME_PREFIXES = []
UNSAFE = set('\\/|:*?"<>')
for c in CHARS:
    if c.isupper() and c.isascii():
        FILENAME_PREFIXES.append(c)
    elif c.islower() and c.isascii():
        FILENAME_PREFIXES.append("l" + c)  # lowercase prefix to distinguish from uppercase
    elif c in "ĄĆĘŁŃÓŚŹŻ":
        names = {"Ą":"pA","Ć":"pC","Ę":"pE","Ł":"pL","Ń":"pN","Ó":"pO","Ś":"pS","Ź":"pZ","Ż":"pZZ"}
        FILENAME_PREFIXES.append(names[c])
    elif c in "ąćęłńóśźż":
        names = {"ą":"plA","ć":"plC","ę":"plE","ł":"plL","ń":"plN","ó":"plO","ś":"plS","ź":"plZ","ż":"plZZ"}
        FILENAME_PREFIXES.append(names[c])
    else:
        punct_names = {
            ".":"PERIOD", ",":"COMMA", "?":"QUESTION", "!":"EXCLAIM",
            '"':"DQUOTE", "'":"SQUOTE", ";":"SEMICOL", ":":"COLON",
            "-":"DASH", "_":"UNDER", "(":"LPAREN", ")":"RPAREN",
            "[":"LBRACK", "]":"RBRACK", "/":"SLASH", "\\":  "BSLASH",
            "@":"AT", "#":"HASH", "$":"DOLLAR", "%":"PERCENT",
            "&":"AMP", "*":"STAR", "+":"PLUS", "=":"EQUAL",
            "<":"LANGLE", ">":"RANGLE"
        }
        FILENAME_PREFIXES.append(punct_names.get(c, "UNK"))

assert len(FILENAME_PREFIXES) == len(CHARS), f"Mismatch: {len(FILENAME_PREFIXES)} prefixes for {len(CHARS)} chars"

# Font paths — add more for variety. Script skips missing ones.
#FONT_PATHS = [
#    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
#    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
#    "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf",
#    "/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf",
#    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
#    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
#    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
#    "/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf",
#    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
#    "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
#    "/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf",
#    "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeMonoOblique.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeMonoBoldOblique.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeSansOblique.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeSansBoldOblique.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeSerif.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeSerifBold.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeSerifBoldItalic.ttf",
#    "/usr/share/fonts/truetype/freefont/FreeSerifItalic.ttf",
#    "/usr/share/fonts/truetype/freefont/andalemo.ttf",
#    "/usr/share/fonts/truetype/freefont/Andale_Mono.ttf",
#    "/usr/share/fonts/truetype/freefont/Arial.ttf",
#    "/usr/share/fonts/truetype/freefont/arialbd.ttf",
#    "/usr/share/fonts/truetype/freefont/arialbi.ttf",
#    "/usr/share/fonts/truetype/freefont/ariali.ttf",
#    "/usr/share/fonts/truetype/freefont/Arial_Black.ttf",
#]

FONT_SIZES = [10, 12, 14, 16, 18, 20, 22, 24, 26]

ttf_files = glob.glob("/usr/share/fonts/**/*.ttf", recursive=True)
ttf_files += glob.glob("/usr/local/share/fonts/**/*.ttf", recursive=True)

loaded_fonts = []
for path in ttf_files:
    try:
        # Test at size 20 first — bitmap fonts will fail here
        ImageFont.truetype(path, 20)
        # If that worked, load all sizes
        for size in FONT_SIZES:
            try:
                loaded_fonts.append(ImageFont.truetype(path, size))
            except Exception:
                pass
    except Exception:
        pass

MAX_FONTS = 1000
random.shuffle(loaded_fonts)
loaded_fonts = loaded_fonts[:MAX_FONTS]

def render_char(char, font):
    """Render a single character centered in a 28x28 white image."""
    img = Image.new("L", (28, 28), color=255)
    draw = ImageDraw.Draw(img)
    bbox = draw.textbbox((0, 0), char, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    x = (28 - w) // 2 - bbox[0]
    y = (28 - h) // 2 - bbox[1]
    draw.text((x, y), char, fill=0, font=font)
    return img

def is_blank(img):
    """Return True if image is entirely white (font doesn't support character)."""
    pixels = list(img.getdata())
    return all(p > 250 for p in pixels)

if not loaded_fonts:
    print("ERROR: No fonts found. Install dejavu-fonts or liberation-fonts.")
    sys.exit(1)

print(f"Loaded {len(loaded_fonts)} font/size combinations.")

counts = {p: 0 for p in FILENAME_PREFIXES}
skipped = 0

for char, prefix in zip(CHARS, FILENAME_PREFIXES):
    for font in loaded_fonts:
        img = render_char(char, font)
        if is_blank(img):
            skipped += 1
            continue
        counts[prefix] += 1
        idx = counts[prefix]
        fname = f"{prefix}_{idx:04d}.png"
        img.save(os.path.join(OUT_DIR, fname))

total = sum(counts.values())
print(f"Generated {total} samples ({skipped} skipped — font missing glyph).")
print("Per-character counts:")
for char, prefix in zip(CHARS, FILENAME_PREFIXES):
    print(f"  {char!r:6s} ({prefix}): {counts[prefix]}")
