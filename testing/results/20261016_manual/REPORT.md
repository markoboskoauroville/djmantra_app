# The manual (V86), 16.10.2026

- **Reader**: Menu -> MANUAL opens `res/qml/djmantra/manual/MANUAL.md` (21 sections: Part 1 using the app, Part 2 the
  technology) with 14 pictures in `manual/images`, rendered from the app's own QML screens at Pixel 7 size (1080x2400,
  sample songs) and the waveform shader. The reader shows Markdown blocks (headings, lists, tables) and each picture with
  its caption. The QML check loads all 42 components and Main.qml.
- **EXPORT MD**: `Documents/DJ Mantra/Manual/DJ Mantra Manual.md` and `images/` beside it.
- **EXPORT PDF**: `Documents/DJ Mantra/Manual/DJ Mantra Manual.pdf`, A4 with 15 mm margins: 22 pages, 26 pictures
  (`DJ Mantra Manual.pdf` here, made by the same steps on the desktop; pages: `pdf_pages.jpg`).
  `QTextDocument::print()` dropped the pictures and changing the image formats afterwards gave blank pages, so
  the pages are painted one by one (`drawContents`) with the pictures given as scaled resources.
- Known limit: a picture can be cut across a page break.
