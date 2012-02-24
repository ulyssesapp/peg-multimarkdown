/**********************************************************************

  markdown_lib.c - markdown in C using a PEG grammar.
  (c) 2008 John MacFarlane (jgm at berkeley dot edu).

  portions Copyright (c) 2010-2011 Fletcher T. Penney

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License or the MIT
  license.  See LICENSE for details.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "markdown_peg.h"

#define TABSTOP 4

/* preformat_text - allocate and copy text buffer while
 * performing tab expansion. */
static GString *preformat_text(const char *text) {
    GString *buf;
    char next_char;
    int charstotab;

    int len = 0;

    buf = g_string_new("");

    charstotab = TABSTOP;
    while ((next_char = *text++) != '\0') {
        switch (next_char) {
        case '\t':
            while (charstotab > 0)
                g_string_append_c(buf, ' '), len++, charstotab--;
            break;
        case '\n':
            g_string_append_c(buf, '\n'), len++, charstotab = TABSTOP;
            break;
        default:
            g_string_append_c(buf, next_char), len++, charstotab--;
        }
        if (charstotab == 0)
            charstotab = TABSTOP;
    }
    g_string_append(buf, "\n\n");
    return(buf);
}

/* print_tree - print tree of elements, for debugging only. */
void print_tree(element * elt, int indent) {
    int i;
    char * key;
    while (elt != NULL) {
        for (i = 0; i < indent; i++)
            fputc(' ', stderr);
        switch (elt->key) {
            case LIST:               key = "LIST"; break;
            case RAW:                key = "RAW"; break;
            case SPACE:              key = "SPACE"; break;
            case LINEBREAK:          key = "LINEBREAK"; break;
            case ELLIPSIS:           key = "ELLIPSIS"; break;
            case EMDASH:             key = "EMDASH"; break;
            case ENDASH:             key = "ENDASH"; break;
            case APOSTROPHE:         key = "APOSTROPHE"; break;
            case SINGLEQUOTED:       key = "SINGLEQUOTED"; break;
            case DOUBLEQUOTED:       key = "DOUBLEQUOTED"; break;
            case STR:                key = "STR"; break;
            case LINK:               key = "LINK"; break;
            case IMAGE:              key = "IMAGE"; break;
            case CODE:               key = "CODE"; break;
            case HTML:               key = "HTML"; break;
            case EMPH:               key = "EMPH"; break;
            case STRONG:             key = "STRONG"; break;
            case PLAIN:              key = "PLAIN"; break;
            case PARA:               key = "PARA"; break;
            case LISTITEM:           key = "LISTITEM"; break;
            case BULLETLIST:         key = "BULLETLIST"; break;
            case ORDEREDLIST:        key = "ORDEREDLIST"; break;
            case H1:                 key = "H1"; break;
            case H2:                 key = "H2"; break;
            case H3:                 key = "H3"; break;
            case H4:                 key = "H4"; break;
            case H5:                 key = "H5"; break;
            case H6:                 key = "H6"; break;
            case BLOCKQUOTE:         key = "BLOCKQUOTE"; break;
            case VERBATIM:           key = "VERBATIM"; break;
            case HTMLBLOCK:          key = "HTMLBLOCK"; break;
            case HRULE:              key = "HRULE"; break;
            case REFERENCE:          key = "REFERENCE"; break;
            case NOTE:               key = "NOTE"; break;
            default:                 key = "?";
        }
        if ( elt->key == STR ) {
            fprintf(stderr, "%p: %s   '%s'\n", elt, key, elt->contents.str);
        } else {
            fprintf(stderr, "%p: %s\n", elt, key);
        }
        if (elt->children)
            print_tree(elt->children, indent + 4);
        elt = elt->next;
    }
}

/* process_raw_blocks - traverses an element list, replacing any RAW elements with
 * the result of parsing them as markdown text, and recursing into the children
 * of parent elements.  The result should be a tree of elements without any RAWs. */
static element * process_raw_blocks(element *input, int extensions, element *references, element *notes, element *labels) {
    element *current = NULL;
    element *last_child = NULL;
    char *contents;
    current = input;

    while (current != NULL) {
        if (current->key == RAW) {
            /* \001 is used to indicate boundaries between nested lists when there
             * is no blank line.  We split the string by \001 and parse
             * each chunk separately. */
            contents = strtok(current->contents.str, "\001");
            current->key = LIST;
            current->children = parse_markdown(contents, extensions, references, notes, labels);
            last_child = current->children;
            while ((contents = strtok(NULL, "\001"))) {
                while (last_child->next != NULL)
                    last_child = last_child->next;
                last_child->next = parse_markdown(contents, extensions, references, notes, labels);
            }
            free(current->contents.str);
            current->contents.str = NULL;
        }
        if (current->children != NULL)
            current->children = process_raw_blocks(current->children, extensions, references, notes, labels);
        current = current->next;
    }
    return input;
}

/* markdown_to_opml_frontend - convert markdown text to OPML
 */
GString * markdown_to_g_string_opml_frontend(const char *text, int extensions) {
    element *result;
    GString *formatted_text;
    GString *out = g_string_new("");
    
    formatted_text = preformat_text(text);
    
    result = parse_markdown_for_opml(formatted_text->str, extensions);
    
    g_string_free(formatted_text, TRUE);
    
    print_element_list(out, result, OPML_FORMAT, extensions);
    
    markdown_free_ast(result);
    
    return out;
}

/* markdown_to_gstring - convert markdown text to the output format specified.
 * Returns a GString, which must be freed after use using g_string_free(). */
GString * markdown_to_g_string(const char *text, int extensions, int output_format) {
    element *ast = markdown_to_ast(text, extensions);
    GString *out = g_string_new("");
    
    print_element_list(out, ast, output_format, extensions);
    
    markdown_free_ast(ast);
    
    return out;
}


/* markdown_to_ast - convert markdown text and return the AST
 */
element *markdown_to_ast(const char *markdownString, int extensions)
{
    element *result;
    element *references;
    element *notes;
    element *labels;
    GString *formatted_text;
    GString *out;
    out = g_string_new("");
    
    formatted_text = preformat_text(markdownString);
    
    references = parse_references(formatted_text->str, extensions);
    notes = parse_notes(formatted_text->str, extensions, references);
    labels = parse_labels(formatted_text->str, extensions, references, notes);
    result = parse_markdown_with_metadata(formatted_text->str, extensions, references, notes, labels);
    
    result = process_raw_blocks(result, extensions, references, notes, labels);
    
    g_string_free(formatted_text, TRUE);

    free_element_list(references);
    free_element_list(labels);
    
    return result;
}

/* markdown_free_ast - Frees the AST after usage
 */
void markdown_free_ast(struct Element *ast)
{
    free_element_list(ast);
}

/* markdown_to_string - convert markdown text to the output format specified.
 * Returns a null-terminated string, which must be freed after use. */
char * markdown_to_string(const char *text, int extensions, int output_format) {
    GString *out;
    char *char_out;
    out = markdown_to_g_string(text, extensions, output_format);
    char_out = out->str;
    g_string_free(out, FALSE);
    return char_out;
}

/* vim:set ts=4 sw=4: */

/* extract_metadata_value - parse document and return value of specified
   metadata key (e.g. "LateX Mode")/
   Returns a null-terminated string, which must be freed after use. */
char * extract_metadata_value(const char *text, int extensions, const char *key) {
    char *value;
    element *result;
    GString *formatted_text;

    formatted_text = preformat_text(text);
    
    result = parse_metadata_only(formatted_text->str, extensions);
    
    value = metavalue_for_key(key, result->children);
    free_element_list(result);
    return value;
}

