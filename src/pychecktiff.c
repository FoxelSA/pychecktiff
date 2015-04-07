/*
 * pychecktiff - Python module to check integrity of JP4/TIFF files
 *
 * Copyright (c) 2015 FOXEL SA - http://foxel.ch
 * Please read <http://foxel.ch/license> for more information.
 *
 *
 * Author(s):
 *
 *      Kevin Velickovic <k.velickovic@foxel.ch>
 *
 *
 * This file is part of the FOXEL project <http://foxel.ch>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Additional Terms:
 *
 *      You are required to preserve legal notices and author attributions in
 *      that material or in the Appropriate Legal Notices displayed by works
 *      containing it.
 *
 *      You are required to attribute the work as explained in the "Usage and
 *      Attribution" section of <http://foxel.ch/license>.
 */

#include "Python.h"
#include <stdio.h>
#include <stdlib.h>
#include <tiffio.h>

#define MAX_MESSAGE_LENGTH 128

/* Global error counter */
int total_errors   = 0;
int total_warnings = 0;
char * Messages[2][16];

/* Function to insert a message */
void insertMessage(const char* message, int type)
{

    /* Determine length of message */
    int message_len = strlen(message);

    if(type == 0)
    {
        /* Allocate memory to store message in array */
        Messages[0][total_errors] = malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
        memset(Messages[0][total_errors], 0x00, MAX_MESSAGE_LENGTH);

        /* Copy message to array */
        strncpy(Messages[0][total_errors], message, message_len);

        /* Increment errors count */
        total_errors++;

    } else if(type == 1) {

        /* Allocate memory to store message in array */
        Messages[1][total_warnings] = malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
        memset(Messages[1][total_warnings], 0x00, MAX_MESSAGE_LENGTH);

        /* Copy message to array */
        strncpy(Messages[1][total_warnings], message, message_len);

        /* Increment warnings count */
        total_warnings++;
    }

}

/* Function to convert messages into a python object */
PyObject * createResults(char * array[2][16], size_t esize, size_t wsize) {

    /* Temp array object */
    PyObject * temp;

    /* Result variable */
    PyObject * result = PyList_New(2);

    /* Loop counter */
    int i = 0;

    /* Add errors to results */
    PyList_SET_ITEM(result, 0, temp = PyList_New(esize));

    for(i = 0; i < esize; i++) {
        PyList_SET_ITEM(temp, i, PyString_FromString(Messages[0][i]));
    }

    /* Add warnings to results */
    PyList_SET_ITEM(result, 1, temp = PyList_New(wsize));

    i = 0;
    for(i = 0; i < wsize; i++) {
        PyList_SET_ITEM(temp, i, PyString_FromString(Messages[1][i]));
    }

    /* Return result */
    return result;
}

/* Function to handle TIFF error messages */
void TIFFError_Handler(const char* module, const char* fmt, va_list argptr)
{
    /* Message buffer */
    char msg[ MAX_MESSAGE_LENGTH ];

    /* Format message */
    vsprintf (msg, fmt, argptr);

    /* Insert message */
    insertMessage( msg, 0 );
}

/* Function to handle TIFF warning messages */
void TIFFWarning_Handler(const char* module, const char* fmt, va_list argptr)
{
    /* Message buffer */
    char msg[ MAX_MESSAGE_LENGTH ];

    /* Format message */
    vsprintf (msg, fmt, argptr);

    /* Insert message */
    insertMessage( msg, 1 );
}

/* The module doc string */
PyDoc_STRVAR(pychecktiff__doc__,
"A tiff file checker");

/* The functions doc strings */
PyDoc_STRVAR(validate_tiff_from_file__doc__,
"Function to check validity of a TIFF file from file");

/* The wrapper to the underlying C function of validate_tiff */
static PyObject *
py_validate_tiff_from_file(PyObject *self, PyObject *args)
{
    /* Arguments containers */
    char* path;

    /* Try to parse arguments */
    if (!PyArg_ParseTuple(args, "s:validate_tiff_from_file", &path))
        return NULL;

    /* Open TIFF file */
    TIFF* tif = TIFFOpen(path, "r");

    /* Check if TIFF file is properly loaded */
    if (tif) {

        /* Image infos containers */
        uint32 imagelength;
        tdata_t buf;
        uint32 row;

        /* Get image length */
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imagelength);

        /* Allocate buffer */
        buf = _TIFFmalloc(TIFFScanlineSize(tif));

        /* Iterate over rows */
        for (row = 0; row < imagelength; row++){

            /* Read row */
            TIFFReadScanline(tif, buf, row, imagelength);
        }

        /* Free buffer */
        _TIFFfree(buf);

        /* CLose image */
        TIFFClose(tif);
    }

    /* Create results object */
    PyObject * results = createResults(Messages, total_errors, total_warnings);

    /* Return result */
    return results;
}

/* Internal python methods bindings */
static PyMethodDef pychecktiff_methods[] = {
    {"validate_tiff_from_file",  py_validate_tiff_from_file, METH_VARARGS, validate_tiff_from_file__doc__},
    {NULL, NULL}      /* sentinel */
};

/* Internal python module initializer */
PyMODINIT_FUNC
initpychecktiff(void)
{

    /* Warning/Error handlers */
    TIFFSetErrorHandler( TIFFError_Handler );
    TIFFSetWarningHandler( TIFFWarning_Handler );

    /* Initialize module */
    Py_InitModule3("pychecktiff", pychecktiff_methods,
                   pychecktiff__doc__);
}
