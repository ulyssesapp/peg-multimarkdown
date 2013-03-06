/* parsing_functions.c - Functions for parsing markdown and
 * freeing element lists. */

/* These yy_* functions come from markdown_parser.c which is
 * generated from markdown_parser.leg
 * */
#include "markdown_parser.c"

#include "utility_functions.h"
#include "parsing_functions.h"
#include "markdown_peg.h"

static void free_element_contents(element elt);

/* free_element_list - free list of elements recursively */
void free_element_list(element * elt) {
    element * next = NULL;
    while (elt != NULL) {
        next = elt->next;
        free_element_contents(*elt);
        if (elt->children != NULL) {
            free_element_list(elt->children);
            elt->children = NULL;
        }
        free(elt);
        elt = next;
    }
}

/* free_element_contents - free element contents depending on type */
static void free_element_contents(element elt) {
    switch (elt.key) {
      case STR:
      case SPACE:
      case RAW:
      case HTMLBLOCK:
      case HTML:
      case VERBATIM:
      case CODE:
      case NOTE:
      case AUTOLABEL:
      case CITATION:
      case TERM:
      case METAKEY:
      case METAVALUE:
      case TABLESEPARATOR:
      case ATTRKEY:
      case GLOSSARY:
      case GLOSSARYTERM:
      case NOTELABEL:
      case CELLSPAN:
      case EMDASH:
      case ENDASH:
      case GLOSSARYSORTKEY:
      case MATHSPAN:
        free(elt.contents.str);
        elt.contents.str = NULL;
        break;
      case LINK:
      case IMAGE:
      case REFERENCE:
        free(elt.contents.link->url);
        elt.contents.link->url = NULL;
        free(elt.contents.link->title);
        elt.contents.link->title = NULL;
        free_element_list(elt.contents.link->label);
        free(elt.contents.link->identifier);
        elt.contents.link->identifier = NULL;
/*        free_element_list(elt.contents.link->attr);*/
        free(elt.contents.link);
        elt.contents.link = NULL;
        break;
      default:
        ;
    }
}

/* free_element - free element and contents */
void free_element(element *elt) {
    free_element_contents(*elt);
    free(elt);
}

yycontext* create_parsing_context() {
	yycontext *context = malloc(sizeof(yycontext));
	memset(context, 0, sizeof(yycontext));
	
	return context;
}

element * parse_references(char *string, int extensions) {

    char *oldcharbuf;
    syntax_extensions = extensions;

	yycontext *context = create_parsing_context();
	
    oldcharbuf = charbuf;
    charbuf = string;
    yyparsefrom(context, yy_References);    /* first pass, just to collect references */
    charbuf = oldcharbuf;

	free(context);
	
    return references;
}

element * parse_notes(char *string, int extensions, element *reference_list) {

    char *oldcharbuf;
    notes = NULL;
    syntax_extensions = extensions;

    if (extension(EXT_NOTES)) {
		yycontext *context = create_parsing_context();
		
        references = reference_list;
        oldcharbuf = charbuf;
        charbuf = string;
        yyparsefrom(context, yy_Notes);     /* second pass for notes */
        charbuf = oldcharbuf;
		
		free(context);
    }

    return notes;
}

element * parse_labels(char *string, int extensions, element *reference_list, element *note_list) {

    char *oldcharbuf;
    syntax_extensions = extensions;
    references = reference_list;
    notes = note_list;
    labels = NULL;

	yycontext *context = create_parsing_context();
	
    oldcharbuf = charbuf;
    charbuf = string;
    yyparsefrom(context, yy_AutoLabels);    /* third pass, to collect labels */
    charbuf = oldcharbuf;

	free(context);
	
    return labels;
}

element * parse_markdown(char *string, int extensions, element *reference_list, element *note_list, element *label_list) {

    char *oldcharbuf;
    syntax_extensions = extensions;
    references = reference_list;
    notes = note_list;
    labels = label_list;

    oldcharbuf = charbuf;
    charbuf = string;

	yycontext *context = create_parsing_context();
	
    yyparsefrom(context, yy_Doc);
	
	free(context);

    charbuf = oldcharbuf;          /* restore charbuf to original value */

/*    if (parse_aborted) {
        free_element_list(parse_result);
        return NULL;
    }*/

    return parse_result;

}

element * parse_markdown_with_metadata(char *string, int extensions, element *reference_list, element *note_list, element *label_list) {

    char *oldcharbuf;
    syntax_extensions = extensions;
    references = reference_list;
    notes = note_list;
    labels = label_list;

    oldcharbuf = charbuf;
    charbuf = string;

	start_time = clock();
	
	yycontext *context = create_parsing_context();
	yyparsefrom(context, yy_DocWithMetaData);
	free(context);
	
    charbuf = oldcharbuf;          /* restore charbuf to original value */

    /* reset start_time for subsequent passes */
    start_time = 0;
    
    if (parse_aborted) {
        parse_aborted = 0;
        free_element_list(parse_result);
        return NULL;
    }

    return parse_result;

}

element * parse_metadata_only(char *string, int extensions) {

    char *oldcharbuf;
    syntax_extensions = extensions;

    oldcharbuf = charbuf;
    charbuf = string;

	yycontext *context = create_parsing_context();
    yyparsefrom(context, yy_MetaDataOnly);
	free(context);
	
    charbuf = oldcharbuf;          /* restore charbuf to original value */
    return parse_result;

}

element * parse_markdown_for_opml(char *string, int extensions) {

    char *oldcharbuf;
    syntax_extensions = extensions;

    oldcharbuf = charbuf;
    charbuf = string;

	yycontext *context = create_parsing_context();
    yyparsefrom(context, yy_DocForOPML);
	free(context);

    charbuf = oldcharbuf;          /* restore charbuf to original value */
    return parse_result;

}
