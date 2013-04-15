/*
   Copyright (C) Cfengine AS

   This file is part of Cfengine 3 - written and maintained by Cfengine AS.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of Cfengine, the applicable Commerical Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include "cf3.defs.h"

#include "xml_writer.h"
#include "manual.h"
#include "files_names.h"
#include "sort.h"
#include "scope.h"
#include "assoc.h"
#include "rlist.h"

static char *MANUAL_DIRECTORY;

#define XMLTAG_DOC_ROOT "syntax-documentation"
#define XMLTAG_VARSCOPES_ROOT "variable-scopes"
#define XMLTAG_FUNCTIONS_ROOT  "functions"
#define XMLTAG_PROMISETYPES_ROOT "promise-types"
#define XMLTAG_CONTROLS_ROOT "controls"
#define XMLTAG_VARSCOPE "variable-scope"
#define XMLTAG_VARIABLE "variable"
#define XMLTAG_LONGDESCRIPTION "long-description"
#define XMLTAG_INTRO "intro"
#define XMLTAG_FUNCTION "function"
#define XMLTAG_DESCRIPTION "description"
#define XMLTAG_ARGUMENT "argument"
#define XMLTAG_EXAMPLE "example"
#define XMLTAG_CONTROL "control"
#define XMLTAG_PROMISETYPE "promise-type"
#define XMLTAG_CONSTRAINTS_ROOT "constraints"
#define XMLTAG_CONSTRAINT "constraint"
#define XMLTAG_DEFAULTVAL "default-value"
#define XMLTAG_TYPE "type"
#define XMLTAG_ACCEPTEDVALS "accepted-values"
#define XMLTAG_RANGE "range"
#define XMLTAG_MIN "min"
#define XMLTAG_MAX "max"
#define XMLTAG_OPTIONS "options"
#define XMLTAG_VALUE "value"

static void XmlExportVariables(Writer *writer, const char *scope);
static void XmlExportFunction(Writer *writer, FnCallType fn);
static void XmlExportPromiseType(Writer *writer, const PromiseTypeSyntax *st);
static void XmlExportControl(Writer *writer, PromiseTypeSyntax body);
static void XmlExportConstraint(Writer *writer, const ConstraintSyntax *bs);
static void XmlExportConstraints(Writer *writer, const ConstraintSyntax *bs);
static void XmlExportType(Writer *writer, DataType dtype, const void *range);

/*****************************************************************************/

void XmlManual(const char *mandir, FILE *fout)
{
    Writer *writer = NULL;
    int i;
    const PromiseTypeSyntax *st = NULL;

    MANUAL_DIRECTORY = (char *) mandir;
    AddSlash(MANUAL_DIRECTORY);

    writer = FileWriter(fout);

/* XML HEADER */
    XmlComment(writer, "AUTOGENERATED SYNTAX DOCUMENTATION BY CF-KNOW");
    WriterWrite(writer, "\n");

/* START XML ELEMENT -- SYNTAX-DOCUMENTATION */
    XmlStartTag(writer, XMLTAG_DOC_ROOT, 0);

/* SPECIAL VARIABLES */
    XmlStartTag(writer, XMLTAG_VARSCOPES_ROOT, 0);
    XmlExportVariables(writer, "const");
    XmlExportVariables(writer, "edit");
    XmlExportVariables(writer, "match");
    XmlExportVariables(writer, "mon");
    XmlExportVariables(writer, "sys");
    XmlExportVariables(writer, "this");
    XmlEndTag(writer, XMLTAG_VARSCOPES_ROOT);

/* SPECIAL FUNCTIONS */
    XmlStartTag(writer, XMLTAG_FUNCTIONS_ROOT, 0);
    for (i = 0; CF_FNCALL_TYPES[i].name != NULL; i++)
    {
        XmlExportFunction(writer, CF_FNCALL_TYPES[i]);
    }
    XmlEndTag(writer, XMLTAG_FUNCTIONS_ROOT);

/* CONTROL */
    XmlStartTag(writer, XMLTAG_CONTROLS_ROOT, 0);
    for (i = 0; CONTROL_BODIES[i].bundle_type != NULL; i++)
    {
        XmlExportControl(writer, CONTROL_BODIES[i]);
    }
    XmlEndTag(writer, XMLTAG_CONTROLS_ROOT);

/* PROMISE TYPES */
    XmlStartTag(writer, XMLTAG_PROMISETYPES_ROOT, 0);
    for (i = 0; i < CF3_MODULES; i++)
    {
        st = CF_ALL_PROMISE_TYPES[i];
        XmlExportPromiseType(writer, st);
    }
    XmlEndTag(writer, XMLTAG_PROMISETYPES_ROOT);

/* END XML ELEMENT -- SYNTAX-DOCUMENTATION */
    XmlEndTag(writer, XMLTAG_DOC_ROOT);

    WriterClose(writer);
}

static void XmlExportVariables(Writer *writer, const char *scope)
{
    char *filebuffer = NULL;
    Rlist *rp = NULL;
    Rlist *list = NULL;

/* START XML ELEMENT -- VARIABLE*-SCOPE */
    XmlAttribute scope_name_attr = { "name", scope };
    XmlStartTag(writer, XMLTAG_VARSCOPE, 1, scope_name_attr);

/* XML ELEMENT -- INTRO */
    filebuffer = ReadTexinfoFileF("varcontexts/%s_intro.texinfo", scope);
    XmlTag(writer, XMLTAG_INTRO, filebuffer, 0);
    free(filebuffer);

    ScopeToList(ScopeGet(scope), &list);
    list = AlphaSortRListNames(list);
    for (rp = list; rp != NULL; rp = rp->next)
    {
        /* START XML ELEMENT -- VARIABLE */
        XmlAttribute var_name_attr = { "name", RlistScalarValue(rp) };
        XmlStartTag(writer, XMLTAG_VARIABLE, 1, var_name_attr);

        /* XML ELEMENT -- LONG-DESCRIPTION */
        filebuffer = ReadTexinfoFileF("vars/%s_%s.texinfo", scope, RlistScalarValue(rp));
        XmlTag(writer, XMLTAG_LONGDESCRIPTION, filebuffer, 0);
        free(filebuffer);

        /* END XML ELEMENT -- VARIABLE */
        XmlEndTag(writer, XMLTAG_VARIABLE);
    }
    RlistDestroy(list);

/* END XML ELEMENT -- VARIABLE-SCOPE */
    XmlEndTag(writer, XMLTAG_VARSCOPE);
}

/*****************************************************************************/

static void XmlExportFunction(Writer *writer, FnCallType fn)
{
    int i;
    char *filebuffer = NULL;

/* START XML ELEMENT -- FUNCTION */
    XmlAttribute fun_name_attr = { "name", fn.name };
    XmlAttribute fun_returntype_attr = { "return-type", CF_DATATYPES[fn.dtype] };
    XmlAttribute fun_varargs_attr = { "varargs", NULL };

    if (fn.varargs)
    {
        fun_varargs_attr.value = "true";
    }
    else
    {
        fun_varargs_attr.value = "false";
    }

    XmlStartTag(writer, XMLTAG_FUNCTION, 3, fun_name_attr, fun_returntype_attr, fun_varargs_attr);

/* XML ELEMENT -- DESCRIPTION */
    XmlTag(writer, XMLTAG_DESCRIPTION, fn.description, 0);

    for (i = 0; fn.args[i].pattern != NULL; i++)
    {
        /* START XML ELEMENT -- ARGUMENT */
        XmlAttribute argument_type_attr = { "type", CF_DATATYPES[fn.args[i].dtype] };
        XmlStartTag(writer, XMLTAG_ARGUMENT, 1, argument_type_attr);

        /* XML ELEMENT -- DESCRIPTION */
        XmlTag(writer, XMLTAG_DESCRIPTION, fn.args[i].description, 0);

        /* END XML ELEMENT -- ARGUMENT */
        XmlEndTag(writer, XMLTAG_ARGUMENT);
    }

/* XML ELEMENT -- LONG-DESCRIPTION */
    filebuffer = ReadTexinfoFileF("functions/%s_notes.texinfo", fn.name);
    XmlTag(writer, XMLTAG_LONGDESCRIPTION, filebuffer, 0);
    free(filebuffer);

/* XML ELEMENT -- EXAMPLE */
    filebuffer = ReadTexinfoFileF("functions/%s_example.texinfo", fn.name);
    XmlTag(writer, XMLTAG_EXAMPLE, filebuffer, 0);
    free(filebuffer);

/* END XML ELEMENT -- FUNCTION */
    XmlEndTag(writer, XMLTAG_FUNCTION);
}

/*****************************************************************************/

static void XmlExportControl(Writer *writer, PromiseTypeSyntax type)
{
    char *filebuffer = NULL;

/* START XML ELEMENT -- CONTROL */
    XmlAttribute control_name_attr = { "name", type.bundle_type };
    XmlStartTag(writer, XMLTAG_CONTROL, 1, control_name_attr);

/* XML ELEMENT -- LONG-DESCRIPTION */
    filebuffer = ReadTexinfoFileF("control/%s_notes.texinfo", type.bundle_type);
    XmlTag(writer, XMLTAG_LONGDESCRIPTION, filebuffer, 0);
    free(filebuffer);

/* XML ELEMENT -- EXAMPLE */
    filebuffer = ReadTexinfoFileF("control/%s_example.texinfo", type.bundle_type);
    XmlTag(writer, XMLTAG_EXAMPLE, filebuffer, 0);
    free(filebuffer);

/* XML ELEMENT -- CONSTRAINTS */
    XmlExportConstraints(writer, type.bs);

/* END XML ELEMENT -- CONTROL */
    XmlEndTag(writer, XMLTAG_CONTROL);
}

/*****************************************************************************/

void XmlExportPromiseType(Writer *writer, const PromiseTypeSyntax *st)
{
    int i;
    char *filebuffer = NULL;

    if (st == NULL)
    {
        return;
    }

    for (i = 0; st[i].bundle_type != NULL; i++)
    {
        /* START XML ELEMENT -- PROMISE TYPE */
        XmlAttribute promise_name_attr = { "name", st[i].promise_type };
        if (strcmp(st[i].promise_type, "*") != 0)
        {
            XmlAttribute promise_agenttype_attr = { "agent-type", NULL };
            if (strcmp(st[i].bundle_type, "*") == 0)
            {
                promise_agenttype_attr.value = "common";
            }
            else
            {
                promise_agenttype_attr.value = st[i].bundle_type;
            }
            XmlStartTag(writer, XMLTAG_PROMISETYPE, 2, promise_name_attr, promise_agenttype_attr);
        }
        else
        {
            XmlStartTag(writer, XMLTAG_PROMISETYPE, 1, promise_name_attr);
        }

        /* XML ELEMENT -- INTRO */
        if (strcmp("*", st[i].bundle_type) == 0)
        {
            filebuffer = ReadTexinfoFileF(MANUAL_DIRECTORY, "promise_common_intro.texinfo");
        }
        else
        {
            filebuffer = ReadTexinfoFileF(MANUAL_DIRECTORY, "promises/%s_intro.texinfo", st[i].promise_type);
        }
        XmlTag(writer, XMLTAG_INTRO, filebuffer, 0);
        free(filebuffer);

        if (strcmp("*", st[i].bundle_type) != 0)
        {
            /* XML ELEMENT -- LONG DESCRIPTION */
            filebuffer = ReadTexinfoFileF(MANUAL_DIRECTORY, "promises/%s_notes.texinfo", st[i].promise_type);
            XmlTag(writer, XMLTAG_LONGDESCRIPTION, filebuffer, 0);
            free(filebuffer);

            /* XML ELEMENT -- EXAMPLE */
            filebuffer = ReadTexinfoFileF(MANUAL_DIRECTORY, "promises/%s_example.texinfo", st[i].promise_type);
            XmlTag(writer, XMLTAG_EXAMPLE, filebuffer, 0);
            free(filebuffer);
        }

        /* EXPORT CONSTRAINTS */
        XmlExportConstraints(writer, st[i].bs);

        /* END XML ELEMENT -- PROMISE TYPE */
        XmlEndTag(writer, XMLTAG_PROMISETYPE);
    }
}

/*****************************************************************************/

void XmlExportConstraints(Writer *writer, const ConstraintSyntax *bs)
{
    int i;

    if (bs == NULL)
    {
        return;
    }

/* START XML ELEMENT -- CONSTRAINTS */
    XmlStartTag(writer, XMLTAG_CONSTRAINTS_ROOT, 0);
    for (i = 0; bs[i].lval != NULL; i++)
    {
        XmlExportConstraint(writer, (const ConstraintSyntax *) &bs[i]);
    }
/* END XML ELEMENT -- CONSTRAINTS */
    XmlEndTag(writer, XMLTAG_CONSTRAINTS_ROOT);
}

/*****************************************************************************/

void XmlExportConstraint(Writer *writer, const ConstraintSyntax *bs)
{
    char *filebuffer = NULL;

    if (bs == NULL)
    {
        return;
    }

/* START XML ELEMENT -- CONSTRAINT */
    XmlAttribute constraint_name_attr = { "name", bs->lval };
    XmlStartTag(writer, XMLTAG_CONSTRAINT, 1, constraint_name_attr);

/* EXPORT TYPE */
    XmlExportType(writer, bs->dtype, bs->range);

/* XML ELEMENT -- DEFAULT-VALUE */
    if (bs->default_value != NULL)
    {
        XmlTag(writer, XMLTAG_DEFAULTVAL, bs->default_value, 0);
    }

    switch (bs->dtype)
    {
    case DATA_TYPE_BODY:
    case DATA_TYPE_BUNDLE:
    case DATA_TYPE_NONE:
    case DATA_TYPE_COUNTER:
        /* NO ADDITIONAL INFO */
        break;

    default:
        /* XML ELEMENT -- DESCRIPTION */
        XmlTag(writer, XMLTAG_DESCRIPTION, bs->description, 0);

        /* XML ELEMENT -- LONG-DESCRIPTION */
        filebuffer = ReadTexinfoFileF(MANUAL_DIRECTORY, "bodyparts/%s_notes.texinfo", bs->lval);
        XmlTag(writer, XMLTAG_LONGDESCRIPTION, filebuffer, 0);
        free(filebuffer);

        /* XML ELEMENT -- EXAMPLE */
        filebuffer = ReadTexinfoFileF(MANUAL_DIRECTORY, "bodyparts/%s_example.texinfo", bs->lval);
        XmlTag(writer, XMLTAG_EXAMPLE, filebuffer, 0);
        free(filebuffer);
    }

/* END XML ELEMENT -- CONSTRAINT */
    XmlEndTag(writer, XMLTAG_CONSTRAINT);
}

/*****************************************************************************/

void XmlExportType(Writer *writer, DataType dtype, const void *range)
{
    Rlist *list = NULL;
    Rlist *rp = NULL;

/* START XML ELEMENT -- TYPE */
    XmlAttribute type_name_attr = { "name", CF_DATATYPES[dtype] };
    XmlStartTag(writer, XMLTAG_TYPE, 1, type_name_attr);

    switch (dtype)
    {
    case DATA_TYPE_BODY:
        /* EXPORT CONSTRAINTS */
        XmlExportConstraints(writer, (ConstraintSyntax *) range);
        break;

    case DATA_TYPE_INT:
    case DATA_TYPE_REAL:
    case DATA_TYPE_INT_LIST:
    case DATA_TYPE_REAL_LIST:
    case DATA_TYPE_INT_RANGE:
    case DATA_TYPE_REAL_RANGE:
        if (range != NULL)
        {
            /* START XML ELEMENT -- RANGE */
            XmlStartTag(writer, XMLTAG_RANGE, 0);

            /* XML ELEMENT -- MIN/MAX */
            int i = 0;

            list = RlistFromSplitString((char *) range, ',');
            for (rp = list; rp != NULL; rp = rp->next, i++)
            {
                if (i == 0)
                {
                    XmlTag(writer, XMLTAG_MIN, RlistScalarValue(rp), 0);
                }
                else
                {
                    XmlTag(writer, XMLTAG_MAX, RlistScalarValue(rp), 0);
                }
            }
            RlistDestroy(list);

            /* END XML ELEMENT -- RANGE */
            XmlEndTag(writer, XMLTAG_RANGE);

            break;
        }

    case DATA_TYPE_OPTION:
    case DATA_TYPE_OPTION_LIST:
        if (range != NULL)
        {
            /* START XML ELEMENT -- OPTIONS */
            XmlStartTag(writer, XMLTAG_OPTIONS, 0);

            /* XML ELEMENT -- VALUE */
            list = RlistFromSplitString((char *) range, ',');
            for (rp = list; rp != NULL; rp = rp->next)
            {
                XmlTag(writer, XMLTAG_VALUE, RlistScalarValue(rp), 0);
            }
            RlistDestroy(list);

            /* END XML ELEMENT -- OPTIONS */
            XmlEndTag(writer, XMLTAG_OPTIONS);

            break;
        }

    case DATA_TYPE_STRING:
    case DATA_TYPE_STRING_LIST:
    case DATA_TYPE_CONTEXT:
    case DATA_TYPE_CONTEXT_LIST:
        /* XML ELEMENT -- ACCEPTED-VALUES */
        if (strlen((char *) range) == 0)
        {
            XmlTag(writer, XMLTAG_ACCEPTEDVALS, "arbitrary string", 0);
        }
        else
        {
            XmlTag(writer, XMLTAG_ACCEPTEDVALS, (char *) range, 0);
        }

        break;

    case DATA_TYPE_BUNDLE:
    case DATA_TYPE_NONE:
    case DATA_TYPE_COUNTER:
        /* NONE */
        break;
    }

/* END XML ELEMENT -- TYPE */
    XmlEndTag(writer, XMLTAG_TYPE);
}
