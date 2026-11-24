#!/usr/bin/env python3
# FREITAKT: builds the Russian user manual as one self-contained HTML file (images inlined as base64 PNG).
# Usage: python3 tools/make_manual_html.py [out.html]
# Screens come from build/ui_shots (the host UI renderer, tests/run_drum_tests.sh); they are saved there at 2x,
# so they are brought back to the LCD's 240x240 (every second pixel) and shown 2x by CSS (image-rendering: pixelated).
import base64, io, os, re, sys
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHOTS = os.path.join(ROOT, 'build', 'ui_shots')
# Options: --lang en builds the English manual (tools/manual_en_content.py) to FREITAKT_manual_EN.html.
_args = sys.argv[1:]
LANG = 'ru'
if '--lang' in _args:
    _i = _args.index('--lang'); LANG = _args[_i + 1]; del _args[_i:_i + 2]
OUT = _args[0] if _args else os.path.join(ROOT, 'build', {'ru': 'FREITAKT_manual.html',
                                                       'en': 'FREITAKT_manual_EN.html'}[LANG])
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import importlib  # noqa: E402
CONTENT = importlib.import_module(f'manual_{LANG}_content').CONTENT  # the text: tools/manual_<lang>_content.py
T = {'ru': {'aria': 'Схема панели FM-1', 'power': 'питание — на верхнем торце', 'toc': 'СОДЕРЖАНИЕ',
            'title': 'FREITAKT — руководство пользователя',
            'sub': 'Прошивка-грувбокс для M-VAVE FM-1 · руководство пользователя'},
     'en': {'aria': 'FM-1 panel schematic', 'power': 'power: on the top edge', 'toc': 'CONTENTS',
            'title': 'FREITAKT — user manual',
            'sub': 'Groovebox firmware for the M-VAVE FM-1 · user manual'}}[LANG]

_cache = {}
def png_b64(name):
    if name not in _cache:
        im = Image.open(os.path.join(SHOTS, name)).convert('RGB')
        if im.size == (480, 480):
            im = im.resize((240, 240), Image.NEAREST)
        buf = io.BytesIO()
        im.save(buf, 'PNG', optimize=True)
        _cache[name] = base64.b64encode(buf.getvalue()).decode()
    return _cache[name]

def shot(name, caption):
    return (f'<figure class="shot"><img alt="{caption}" src="data:image/png;base64,{png_b64(name)}">'
            f'<figcaption>{caption}</figcaption></figure>')

# ---- the panel schematic: the FM-1's landscape geometry from web/sim/layout.js (design units 1000 x 620)
NAMES = ['F', 'F#', 'G', 'G#', 'A', 'A#', 'B', 'C', 'C#', 'D', 'D#', 'E']
def panel_svg():
    o = ['<svg class="panel" viewBox="0 0 1000 620" xmlns="http://www.w3.org/2000/svg" role="img" '
         f'aria-label="{T["aria"]}">',
         '<rect x="4" y="4" width="992" height="612" rx="40" fill="#1b1b1b"/>',
         '<rect x="270" y="36" width="268" height="268" rx="10" fill="#333"/>',
         f'<image x="284" y="50" width="240" height="240" style="image-rendering:pixelated" '
         f'href="data:image/png;base64,{png_b64("24_tracks.png")}"/>',
         '<rect x="572" y="158" width="394" height="156" rx="12" fill="#2a2a2a"/>',
         '<rect x="52" y="262" width="196" height="64" rx="12" fill="#2a2a2a"/>',
         '<rect x="40" y="336" width="920" height="262" rx="14" fill="#2a2a2a"/>']
    def knob(cx, cy, label, accent=False):
        col = '#ff6000' if accent else '#888'
        o.append(f'<circle cx="{cx}" cy="{cy}" r="23" fill="#444" stroke="{col}" stroke-width="3"/>'
                 f'<line x1="{cx}" y1="{cy}" x2="{cx}" y2="{cy-18}" stroke="#ddd" stroke-width="3"/>'
                 f'<text x="{cx}" y="{cy+40}" class="pl">{label}</text>')
    knob(95, 108, 'MASTER'); knob(195, 108, 'SELECT', True)
    knob(95, 214, 'PRESETS'); knob(195, 214, 'ALGORITHM', True)
    for i, x in enumerate((632, 731, 830, 929)):
        knob(x, 108, f'KNOB {i+1}', True)
    rows = [['FX', 'SEL', 'ENV', 'LFO', 'EDIT', 'GLO'], ['HOME', 'SAVE', 'ARP', 'SEQ', 'PLAY', 'REC']]
    for j, r in enumerate(rows):
        for i, l in enumerate(r):
            x, y = 588 + i * 62, 174 + j * 68
            o.append(f'<rect x="{x}" y="{y}" width="52" height="52" rx="8" fill="#ff6000"/>'
                     f'<text x="{x+26}" y="{y+31}" class="pb">{l}</text>')
    for l, x in (('OCT-', 66), ('OCT+', 158)):
        o.append(f'<rect x="{x}" y="275" width="76" height="38" rx="8" fill="#ff6000"/>'
                 f'<text x="{x+38}" y="299" class="pb">{l}</text>')
    keys = [(n, NAMES[n % 12]) for n in range(27)]
    whites = [n for n, nm in keys if '#' not in nm]
    step = 880 / len(whites)
    for i, n in enumerate(whites):
        x = 60 + i * step + (step - 47) / 2
        o.append(f'<rect x="{x:.1f}" y="464" width="47" height="122" rx="5" fill="#eee"/>'
                 f'<text x="{x+23.5:.1f}" y="572" class="kw">{i+1}</text>')
    bi = 0
    for n, nm in keys:
        if '#' in nm:
            left = whites.index(n - 1)
            x = 60 + (left + 1) * step - 22
            bi += 1
            fill = '#ff6000' if bi <= 6 else '#111'
            tc = '#000' if bi <= 6 else '#ccc'
            o.append(f'<rect x="{x:.1f}" y="350" width="44" height="100" rx="5" fill="{fill}" stroke="#555"/>'
                     f'<text x="{x+22:.1f}" y="438" class="kb" fill="{tc}">{bi}</text>')
    o.append(f'<text x="500" y="20" class="pl" fill="#aaa">{T["power"]}</text>')
    o.append('</svg>')
    return ''.join(o)

CSS = r'''
:root{--or:#ff6000;--ink:#111;--paper:#fff;--soft:#fff3ea;--line:#e6c9b4}
*{box-sizing:border-box}
html{scroll-behavior:smooth}
body{margin:0;font:16px/1.55 "Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;color:var(--ink);background:var(--paper)}
header.top{background:var(--or);color:#000;padding:28px 32px 22px;border-bottom:4px solid #000}
header.top h1{margin:0;font:800 44px/1.1 "Consolas","Courier New",monospace;letter-spacing:4px}
header.top p{margin:6px 0 0;font-size:17px}
.wrap{display:grid;grid-template-columns:270px 1fr;max-width:1320px;margin:0 auto}
nav.toc{position:sticky;top:0;align-self:start;max-height:100vh;overflow:auto;padding:18px 14px 40px 22px;
  border-right:1px solid var(--line);font-size:14px;background:#fffaf6}
nav.toc b{display:block;margin:0 0 8px;font-size:12px;letter-spacing:2px;color:#a04000}
nav.toc ol{margin:0;padding-left:20px}
nav.toc li{margin:3px 0}
nav.toc ol ol{padding-left:14px;font-size:13px}
nav.toc a{color:#000;text-decoration:none}
nav.toc a:hover{color:var(--or);text-decoration:underline}
main{padding:10px 40px 80px;min-width:0}
h2{margin:46px 0 14px;padding:8px 14px;background:var(--or);color:#000;font-size:26px;border-radius:4px}
h3{margin:30px 0 8px;font-size:20px;border-bottom:2px solid var(--or);padding-bottom:3px}
h4{margin:20px 0 6px;font-size:16px}
a{color:#b84500}
kbd{display:inline-block;padding:0 6px;border:1.5px solid #000;border-bottom-width:3px;border-radius:4px;
  background:var(--or);color:#000;font:700 13px/1.5 "Consolas","Courier New",monospace;white-space:nowrap}
kbd.k{background:#fff}
code,.mono{font-family:"Consolas","Courier New",monospace;background:var(--soft);padding:0 4px;border-radius:3px}
table{border-collapse:collapse;width:100%;margin:10px 0 18px;font-size:15px}
th,td{border:1px solid var(--line);padding:6px 9px;text-align:left;vertical-align:top}
th{background:#ffe2cc}
tr:nth-child(even) td{background:#fffaf6}
.note,.warn{padding:10px 14px;border-left:5px solid var(--or);background:var(--soft);margin:14px 0;border-radius:3px}
.warn{border-left-color:#c00;background:#fff0f0}
.shots{display:flex;flex-wrap:wrap;gap:18px;margin:10px 0 16px}
figure.shot{margin:0;width:244px}
figure.shot img{display:block;width:240px;height:240px;image-rendering:pixelated;border:2px solid #000;border-radius:4px}
figure.shot.big img{width:480px;height:480px}
figure.shot.big{width:484px}
figure.shot figcaption{font-size:13px;color:#444;margin-top:4px}
.screen{display:grid;grid-template-columns:484px 1fr;gap:24px;align-items:start;margin:12px 0 22px}
.screen figure.shot{width:484px}
.screen figure.shot img{width:480px;height:480px}
svg.panel{width:100%;max-width:1000px;display:block;margin:10px 0}
svg.panel .pl{font:700 15px Consolas,monospace;fill:#ddd;text-anchor:middle}
svg.panel .pb{font:800 15px Consolas,monospace;fill:#000;text-anchor:middle}
svg.panel .kw{font:700 15px Consolas,monospace;fill:#555;text-anchor:middle}
svg.panel .kb{font:700 15px Consolas,monospace;text-anchor:middle}
.ex{font-family:Consolas,monospace;font-size:14px;line-height:1.35;background:#111;color:#ffb37f;padding:12px 16px;
  border-radius:6px;overflow:auto;white-space:pre}
.ex b{color:#fff}
@media (max-width:1100px){.wrap{grid-template-columns:1fr}nav.toc{position:static;max-height:none;border:0}
  .screen{grid-template-columns:1fr}}
@media print{
  nav.toc{display:none}.wrap{display:block}main{padding:0}
  header.top{-webkit-print-color-adjust:exact;print-color-adjust:exact}
  h2,kbd,th,svg.panel,.ex{-webkit-print-color-adjust:exact;print-color-adjust:exact}
  h2{break-before:page}h2.nobreak{break-before:auto}
  figure,table,.screen,.note,.warn{break-inside:avoid}
  .screen{grid-template-columns:250px 1fr}
  .screen figure.shot,figure.shot.big{width:244px}
  .screen figure.shot img,figure.shot.big img{width:240px;height:240px}
  a{color:#000;text-decoration:none}
}
'''

def build():
    body = CONTENT
    body = body.replace('{{PANEL}}', panel_svg())
    body = re.sub(r'\{\{shot:([^|}]+)\|([^}]*)\}\}', lambda m: shot(m.group(1), m.group(2)), body)
    def screen(m):
        return f'<div class="screen">{shot(m.group(1), m.group(2))}<div>'
    body = re.sub(r'\{\{screen:([^|}]+)\|([^}]*)\}\}', screen, body)
    body = body.replace('{{/screen}}', '</div></div>')
    # the table of contents from the h2 / h3 with ids
    toc, open3 = [f'<nav class="toc"><b>{T["toc"]}</b><ol>'], False
    for lvl, hid, title in re.findall(r'<h([23]) id="([^"]+)"[^>]*>(.*?)</h\1>', body):
        title = re.sub('<[^>]+>', '', title)
        if lvl == '2':
            if open3: toc.append('</ol>'); open3 = False
            toc.append(f'</li><li><a href="#{hid}">{title}</a>')
        else:
            if not open3: toc.append('<ol>'); open3 = True
            toc.append(f'<li><a href="#{hid}">{title}</a></li>')
    if open3: toc.append('</ol>')
    toc.append('</li></ol></nav>')
    toc_html = ''.join(toc).replace('<ol></li>', '<ol>', 1)
    html = (f'<!DOCTYPE html><html lang="{LANG}"><head><meta charset="utf-8">'
            '<meta name="viewport" content="width=device-width,initial-scale=1">'
            f'<title>{T["title"]}</title><style>' + CSS + '</style></head><body>'
            f'<header class="top"><h1>FREITAKT</h1><p>{T["sub"]}</p></header>'
            '<div class="wrap">' + toc_html + '<main>' + body + '</main></div></body></html>')
    with open(OUT, 'w', encoding='utf-8') as f:
        f.write(html)
    print(OUT, len(html.encode()), 'bytes', len(_cache), 'screens')

if __name__ == '__main__':
    build()
