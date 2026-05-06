#!/usr/bin/env python3
"""
Generate a Bangla (Bengali) text test EPUB for verifying rendering on the X4.

Tests:
- Basic Bangla vowels and consonants
- Pre-base matras (ি U+09BF, ে U+09C7) — must appear to the left of the consonant
- Conjuncts via virama (্ U+09CD) e.g. ক্ষ, ব্য, ত্ত
- Real Bangla words and sentences
- Mixed Bangla + English text
- Numbers (Bengali digits)
"""

import zipfile
from pathlib import Path

OUTPUT_DIR = Path(__file__).parent.parent / "test" / "epubs"


def make_chapter(title, body_html):
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
<head><title>{title}</title></head>
<body>
<h1>{title}</h1>
{body_html}
</body>
</html>'''


def create_epub(epub_path, title, chapters):
    with zipfile.ZipFile(epub_path, 'w', zipfile.ZIP_DEFLATED) as epub:
        epub.writestr('mimetype', 'application/epub+zip', compress_type=zipfile.ZIP_STORED)

        epub.writestr('META-INF/container.xml', '''<?xml version="1.0" encoding="UTF-8"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles>
    <rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/>
  </rootfiles>
</container>''')

        manifest_items = []
        spine_items = []

        for i, (chapter_title, html_content) in enumerate(chapters):
            chapter_id = f'chapter{i + 1}'
            chapter_file = f'chapter{i + 1}.xhtml'
            manifest_items.append(
                f'    <item id="{chapter_id}" href="{chapter_file}" media-type="application/xhtml+xml"/>'
            )
            spine_items.append(f'    <itemref idref="{chapter_id}"/>')
            epub.writestr(f'OEBPS/{chapter_file}', html_content)

        epub.writestr('OEBPS/content.opf', f'''<?xml version="1.0" encoding="UTF-8"?>
<package xmlns="http://www.idpf.org/2007/opf" version="3.0" unique-identifier="uid">
  <metadata xmlns:dc="http://purl.org/dc/elements/1.1/">
    <dc:identifier id="uid">test-epub-bangla</dc:identifier>
    <dc:title>{title}</dc:title>
    <dc:language>bn</dc:language>
  </metadata>
  <manifest>
    <item id="nav" href="nav.xhtml" media-type="application/xhtml+xml" properties="nav"/>
{chr(10).join(manifest_items)}
  </manifest>
  <spine>
{chr(10).join(spine_items)}
  </spine>
</package>''')

        nav_items = '\n'.join(
            f'      <li><a href="chapter{i + 1}.xhtml">{chapters[i][0]}</a></li>'
            for i in range(len(chapters))
        )
        epub.writestr('OEBPS/nav.xhtml', f'''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:epub="http://www.idpf.org/2007/ops">
<head><title>বিষয়সূচি</title></head>
<body>
  <nav epub:type="toc">
    <h1>বিষয়সূচি</h1>
    <ol>
{nav_items}
    </ol>
  </nav>
</body>
</html>''')


def main():
    OUTPUT_DIR.mkdir(exist_ok=True)

    chapters = [
        # --- Chapter 1: Basic vowels and consonants ---
        ("স্বরবর্ণ ও ব্যঞ্জনবর্ণ", make_chapter("স্বরবর্ণ ও ব্যঞ্জনবর্ণ", '''
<p>স্বরবর্ণ (Vowels):</p>
<p>অ আ ই ঈ উ ঊ ঋ এ ঐ ও ঔ</p>
<p>ব্যঞ্জনবর্ণ (Consonants):</p>
<p>ক খ গ ঘ ঙ</p>
<p>চ ছ জ ঝ ঞ</p>
<p>ট ঠ ড ঢ ণ</p>
<p>ত থ দ ধ ন</p>
<p>প ফ ব ভ ম</p>
<p>য র ল শ ষ স হ</p>
<p>ড় ঢ় য় ৎ ং ঃ ঁ</p>
''')),

        # --- Chapter 2: Pre-base matras (critical shaping test) ---
        ("প্রি-বেস মাত্রা পরীক্ষা", make_chapter("প্রি-বেস মাত্রা পরীক্ষা", '''
<p>ি-কার (U+09BF) — মাত্রা বাম দিকে যাবে:</p>
<p>কি খি গি ঘি চি ছি জি ঝি</p>
<p>টি ঠি ডি ঢি তি থি দি ধি নি</p>
<p>পি ফি বি ভি মি যি রি লি শি সি হি</p>
<p></p>
<p>ে-কার (U+09C7) — মাত্রা বাম দিকে যাবে:</p>
<p>কে খে গে ঘে চে ছে জে ঝে</p>
<p>টে ঠে ডে ঢে তে থে দে ধে নে</p>
<p>পে ফে বে ভে মে যে রে লে শে সে হে</p>
<p></p>
<p>PASS: ি and ে appear to the LEFT of each consonant.</p>
<p>FAIL: ি or ে appears to the RIGHT (shaping not applied).</p>
''')),

        # --- Chapter 3: Conjuncts ---
        ("যুক্তবর্ণ পরীক্ষা", make_chapter("যুক্তবর্ণ পরীক্ষা", '''
<p>সাধারণ যুক্তবর্ণ (Common conjuncts):</p>
<p>ক্ষ ত্র জ্ঞ হ্ম ষ্ট ষ্ণ</p>
<p>ন্ত ন্দ ন্ধ ন্ব ন্ম</p>
<p>ব্য ক্য ত্য সত্য</p>
<p>ত্ত ত্ত্ব</p>
<p>স্ত স্থ</p>
<p></p>
<p>বিরাম-সহ (With visible virama ্):</p>
<p>ক্‍ষ → ক্ষ (should merge into one conjunct glyph)</p>
<p></p>
<p>PASS: Each pair above renders as a single merged conjunct glyph.</p>
<p>FAIL: Pairs render as two separate letters with a virama between them.</p>
''')),

        # --- Chapter 4: Real words ---
        ("বাংলা শব্দ", make_chapter("বাংলা শব্দ", '''
<p>সাধারণ শব্দ:</p>
<p>বাংলাদেশ ভারত কলকাতা ঢাকা</p>
<p>মানুষ পানি আকাশ মাটি</p>
<p>বই পড়া লেখা শেখা</p>
<p></p>
<p>সংখ্যা (Numbers):</p>
<p>০ ১ ২ ৩ ৪ ৫ ৬ ৭ ৮ ৯</p>
<p>১০ ২০ ৫০ ১০০ ১০০০</p>
<p></p>
<p>জটিল শব্দ (Complex words with conjuncts):</p>
<p>সাহিত্য বাক্য সংখ্যা বিদ্যালয়</p>
<p>স্বাধীনতা প্রতিষ্ঠান বিশ্ববিদ্যালয়</p>
<p>আন্তর্জাতিক সম্পর্ক</p>
''')),

        # --- Chapter 5: Sentences and paragraphs ---
        ("বাক্য ও অনুচ্ছেদ", make_chapter("বাক্য ও অনুচ্ছেদ", '''
<p>আমার সোনার বাংলা, আমি তোমায় ভালোবাসি।</p>
<p>চিরদিন তোমার আকাশ, তোমার বাতাস, আমার প্রাণে বাজায় বাঁশি।</p>
<p></p>
<p>বাংলাদেশ দক্ষিণ এশিয়ার একটি রাষ্ট্র। এর রাজধানী ঢাকা।
বাংলাদেশের জাতীয় ভাষা বাংলা। দেশটির আয়তন প্রায় ১,৪৭,৫৭০ বর্গকিলোমিটার।</p>
<p></p>
<p>পৃথিবীতে প্রায় ২৩ কোটি মানুষ বাংলা ভাষায় কথা বলেন।
বাংলা ভাষার নিজস্ব বর্ণমালা রয়েছে এবং এটি বিশ্বের অন্যতম সমৃদ্ধ ভাষা।</p>
''')),

        # --- Chapter 6: Mixed Bangla + English ---
        ("মিশ্র পাঠ্য", make_chapter("মিশ্র পাঠ্য (Mixed Text)", '''
<p>Bangla and English in the same paragraph:</p>
<p>বাংলাদেশ (Bangladesh) is a country in South Asia.
এর রাজধানী ঢাকা (Dhaka), which has a population of about ২ কোটি (20 million).</p>
<p></p>
<p>এই device-এ Bangla font ঠিকমতো কাজ করছে কিনা তা যাচাই করা হচ্ছে।</p>
<p>Font size: ১৪px (medium), ১৬px (large).</p>
<p></p>
<p>PASS: Bangla and English both render on the same line without garbling.</p>
''')),
    ]

    output_path = OUTPUT_DIR / 'test_bangla.epub'
    create_epub(output_path, 'বাংলা ফন্ট পরীক্ষা', chapters)
    print(f"Created: {output_path}")


if __name__ == '__main__':
    main()
