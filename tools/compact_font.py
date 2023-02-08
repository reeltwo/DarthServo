"""
I think this example code very quickly explains how this modlue can be used and
can serve as a cookie cutter code that you can copy and paste.
"""

import re
import sys
import glob
import os.path
import fontTools.subset
from html.parser import HTMLParser

def load_the_font(load_font_path):
    "Function to load a font, font must be TrueType or OpenType font e.g TFF, WOFF"
    options = fontTools.subset.Options()
    return fontTools.subset.load_font(load_font_path, options)


def get_glyphs_from_file(file_name):
    "Obtain the glyphs that want to be kept from a line seperated file"
    with open(file_name, 'r') as file:
        return file.read().splitlines()


def load_and_crush_font(font_path, glyphs):
    "Load a font then compress it to keep only needed glyphs"
    original_font = load_the_font(font_path)
    return crush_font(original_font, glyphs)


def crush_font(font_object, glyphs):
    "Function to strip unneeded glyphs from a font object"
    options = fontTools.subset.Options()
    options.desubroutinize = True
    options.glyph_names = True
    # Create a subset of the font
    subsetter = fontTools.subset.Subsetter(options=options)
    subsetter.populate(glyphs=set(glyphs))
    subsetter.subset(font_object)
    return font_object

def read_codepoint(file_name):
    "Function to read the codepoint file"
    codepoints = {}
    with open(file_name, 'r') as file:
        for line in file:
            words = line.split()
            codepoints.update({words[0]: int(words[1], 16)})
    return codepoints

def get_character_mapping(font_object):
    "Function to get the character mapping from the unicode value to the glyph name"
    # Get character mapping from the cmap table
    char_mapping = [c.cmap.items() for c in font_object["cmap"].tables]
    # Now flatten list by one layer and create a dictrionary
    return dict(item for sublist in char_mapping for item in sublist)


def get_re_for_font_glyph_unicode(font_abreviation, glyph):
    "Function to get the regex for glyph unicode value"
    font_awesome_regex = [r'(?i)\.{}-'.format(font_abreviation),
                          """:before[^{}]*{\s*content:\s*['"]([^'"]*)['"][^\}]*\}"""]

    return ('|'.join([glyph])).join(font_awesome_regex)


def get_unicode_value(glyph, codepoint_file, sed_out):
    "Returns the unicode value for the glyph requested"
    try:
        print('glyph', glyph, ':', "U+%04X" % codepoint_file[glyph])
        sed_out.write("s?<span class=\"syms\">{}</span>?<span class=\"syms\">\\&#x{:04x};</span>?\n".format(glyph, codepoint_file[glyph]));
        return codepoint_file[glyph]
    except IndexError:
        raise KeyError('The glyph "{}" was not found in the codepoint file'.format(glyph))


def get_unicode_value_list(glyph_list, codepoint_file, sed_out):
    "Returns the unicode mapping for a list of glyphs in a dictionary"
    return {glyph: get_unicode_value(glyph, codepoint_file, sed_out) for glyph in glyph_list}


def get_glyph_name(character_map, unicode_value):
    "Get the name of the glyph from the unicode value"
    return character_map[unicode_value]


def get_glyph_name_list(font_object, unicode_value_list):
    "Get a dictionary of the unicode chracter to the glyph name for a list of glyphs"
    character_map = get_character_mapping(font_object)
    return {u_value: get_glyph_name(character_map, u_value) for u_value in unicode_value_list}


def remove_glyphs_from_css(input_css, font_type, glyphs):
    "Remove CSS for unwanted glyphs"
    re_patterns = get_re_for_glyphs(font_type, glyphs)
    for re_pattern, re_replacement in re_patterns:
        input_css = re.sub(re_pattern, re_replacement, input_css)
    return re.sub(r'\n{3,}', '\n\n', input_css)


def get_re_for_glyphs(font_type, glyphs):
    """Function to get the regex used for removing unwanted glyphs
    If a custom font is used put in the fonts css abbreviation for the font_type
    """
    # All font names and font abbreviations with font abbreviations used
    font_types_abbreviations = {
        'font_awesome': 'fa',
        'foundation_icon': 'fi',
        'material_design': 'mdi',
        'glyphicon': 'glyphicon',  # Useless but for completion
        'ionicons': 'ion',
        'elusive': 'el'
    }
    return get_re_for_font_glyphs(font_types_abbreviations.get(font_type, font_type), glyphs)


def get_re_for_font_glyphs(font_abbreviation, glyphs):
    "Function to get the regex for unwanted glyphs"
    # Only keep used glyphs
    font_awesome_regex = [r'(?i)(?<=}})\s*(\.{}-(?!('.format(font_abbreviation),
                          r'):)[0-9a-z\-]*:before,\s*)*\.{}-(?!('.format(font_abbreviation),
                          r'):)[0-9a-z-]*:before\s*{\s*content:\s*"\\?\w+"[^\}]*\}']
    # Also remove unused aliases
    unused_selector = [r'(?i)\.{}-(?!('.format(font_abbreviation),
                       r'):before)[^.]*?:before,?']

    return [(('|'.join(glyphs)).join(font_awesome_regex), ''),
            (('|'.join(glyphs)).join(unused_selector), '')]

class MyHTMLParser(HTMLParser):
    glyphs_used = set()
    span_tag = False

    def handle_starttag(self, tag, attrs):
        self.span_tag = (tag == 'span')

    def handle_endtag(self, tag):
        self.span_tag = False

    def handle_data(self, data):
        if self.span_tag:
            print('used_tag: ', data)
            self.glyphs_used.update([data])

def find_used_glyphs(include_args, regex_pattern, exclude_args=[]):
    """
    This function will search through the files defined by the glob args
    you can easily include all files in a directory and subdirectories
    using wild cards. Make sure you exclude the font's css file as it
    will in some fonts have the exact same names as the regex used to match.

    This function works somewhat simmilarly to grep.

    include_args should be a list of glob arguments that are also in a list.
    Each glob argument in that list has to be a string and can optionally be
    followed by a dictionary with the kwargs e.g {'recursive': True}.
    """

    exclude_files = set()
    for exclude_arg in exclude_args:
        # If no kwargs exists append them
        if len(exclude_arg) == 1:
            exclude_arg.append({})
        for file in glob.iglob(exclude_arg[0], **exclude_arg[1]):
            print('exclude:', file)
            exclude_files.update([file])

    # Set because glyphs will be repeated
    parser = MyHTMLParser()

    for include_arg in include_args:
        # If no kwargs exists append them
        if len(include_arg) == 1:
            include_arg.append({})
        for file in glob.iglob(include_arg[0], **include_arg[1]):
            # Check the item is a file and is also one which is not excluded
            if os.path.isfile(file) and file not in exclude_files:
                try:
                    print('include:', file)
                    with open(file, 'r') as file_obj:
                        for line in file_obj:
                            parser.feed(line)
                            # search = re.search(regex_pattern, line)
                            # if search:
                            #     glyphs_used.update([search.group(1)])
                except UnicodeDecodeError:
                    pass  # File is not text (binary files)
                except:
                    raise
    return parser.glyphs_used


def get_regex_for_glyph(font_name):
    "A function to return the regex for each font"

    font_types_regex = {
        'font_awesome': 'fa fa-([a-zA-Z0-9\-]+)',
        'foundation_icon': 'fi-([a-zA-Z0-9\-]+)',
        'material_design': 'mdi mdi-([a-zA-Z0-9\-]+)',
        'glyphicon': 'glyphicons glyphicons-([a-zA-Z0-9\-]+)',
        'ionicons': 'icon ion-([a-zA-Z0-9\-]+)',
        'elusive': 'el el-([a-zA-Z0-9\-]+)'
    }

    return font_types_regex[font_name]


# Load in font and CSS file for font
font_object = load_the_font('html/site/font.woff2')
codepoint_file = read_codepoint('html/site/font.codepoint')
sed_out = open('html/site/font.sed', 'w')

# Find all glyphs used in website
regex_for_find = get_regex_for_glyph('material_design')
include_argument = [['./html/site/**/*.html', {'recursive': True}]]
exclude_arg = [['html/site/subdirectory/exclude_file.txt']]
used_glyphs = find_used_glyphs(include_argument, regex_for_find, exclude_args=exclude_arg)

# Find the unicode values of those glyphs and use unicode value to get glyph name
unicode_values = get_unicode_value_list(used_glyphs, codepoint_file, sed_out).values()
glyph_names = get_glyph_name_list(font_object, unicode_values).values()

# create a new font object with only the glyphs used in the website
crushed_font = crush_font(font_object, glyph_names)

# Save the crushed font and save the crushed CSS
#crushed_font.save(r'./html/font.crushed.ttf')

# To save a woff font
crushed_font.flavor = 'woff2'
crushed_font.save('html/site/font.mini.woff2')
