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

#define MAX_MESSAGE_LENGTH 512

/* Global error counter */
int total_errors   = 0;
int total_warnings = 0;
char * Messages[2][16];

/* Memory file structure */
typedef struct memory_file_struct {
    unsigned char* data;
    int length;
    int seek_state;
} memory_file;

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

    /* Flush error messages */
    int x = 0;
    for( x = 0; x < total_errors; x++ ){
        free( Messages[ 0 ][ x ] );
    }
    total_errors = 0;

    /* Flush warning messages */
    for( x = 0; x < total_warnings; x++ ){
        free( Messages[ 1 ][ x ] );
    }
    total_warnings = 0;

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


/* Function to read tiff file */
static tsize_t tiff_Read(thandle_t fd, tdata_t data, tsize_t size)
{
    /* Cast memory file */
    memory_file* handle = (memory_file *) fd;

    /* Cursor clamping */
    if ((size + handle->seek_state) > handle->length) {
	   size = handle->length - handle->seek_state;
    }

    /* Check if size is ok */
    if (size) {

        /* Copy data region */
    	memcpy((unsigned char *) data, handle->data + handle->seek_state, (size_t) size);

        /* Update seek_state */
    	handle->seek_state += size;
    }

    /* Return result */
    return size;
}

/* Function to write tiff file (dummy) */
static tsize_t tiff_Write(thandle_t fd, tdata_t data, tsize_t size)
{
    /* Return result */
    return size;
}

/* Function to seek into buffer */
static toff_t tiff_Seek(thandle_t fd, toff_t off, int whence)
{
    /* Cast memory file */
    memory_file *handle = (memory_file *) fd;

    /* Detect SEEK type & set proper state */
    switch (whence) {
    	case SEEK_SET:
    	    handle->seek_state = (int) off;
    	    break;
    	case SEEK_CUR:
    	    handle->seek_state += (int) off;
    	    break;
    	case SEEK_END:
    	    handle->seek_state = handle->length + (int) off;
    	    break;
    }

    /* Check if seek are ok */
    if (handle->seek_state < 0) {
        handle->seek_state = 0;
        return -1;
    }

    /* Return result */
    return (toff_t) handle->seek_state;
}

/* Function to close tiff file (dummy) */
static int tiff_Close(thandle_t fd)
{
    return 0;
}

/* Function to get tiff size */
static toff_t tiff_Size(thandle_t fd)
{
    return ((memory_file *) fd)->length;
}

/* The module doc string */
PyDoc_STRVAR(pychecktiff__doc__,
"A tiff file checker");

/* The functions doc strings */
PyDoc_STRVAR(validate_tiff_from_file__doc__,
"Function to check validity of a TIFF file from file");

PyDoc_STRVAR(validate_tiff_from_buffer__doc__,
"Function to check validity of a TIFF file from buffer");

void scan_tiff( TIFF* tiff_file )
{
    /* Image infos containers */
    uint32 imagelength;
    tdata_t buf;
    uint32 row;

    /* Get image length */
    TIFFGetField(tiff_file, TIFFTAG_IMAGELENGTH, &imagelength);

    /* Allocate buffer */
    buf = _TIFFmalloc(TIFFScanlineSize( tiff_file ));

    /* Iterate over rows */
    for (row = 0; row < imagelength; row++){

        /* Read row */
        TIFFReadScanline(tiff_file, buf, row, imagelength);
    }

    /* Free buffer */
    _TIFFfree(buf);
}

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

        /* Scan tiff file */
        scan_tiff( tif );

        /* CLose image */
        TIFFClose(tif);
    }

    /* Create results object */
    PyObject * results = createResults(Messages, total_errors, total_warnings);

    /* Return result */
    return results;
}

/* The wrapper to the underlying C function for validate_tiff_from_buffer */
static PyObject *
py_validate_tiff_from_buffer(PyObject *self, PyObject *args)
{
	/* Arguments containers */
	unsigned char * buffer;
	int buffer_length = 0;

	/* Try to parse arguments */
	if (!PyArg_ParseTuple(args, "s#:validate_tiff_from_buffer", &buffer, &buffer_length))
		return NULL;

    /* Initialize memory file container */
    memory_file* main_file_buffer = (memory_file*)malloc( sizeof( memory_file ) );

    /* Assign values to memory file */
    main_file_buffer->data = buffer;
    main_file_buffer->length = buffer_length;
    main_file_buffer->seek_state = 0;

    /* Open TIFF file from memory */
    TIFF* tif = TIFFClientOpen("inline data", "r", (thandle_t) main_file_buffer,
    tiff_Read, tiff_Write, tiff_Seek, tiff_Close,
    tiff_Size, NULL, NULL);

    /* Check if TIFF file is properly loaded */
    if (tif) {

        /* Scan tiff file */
        scan_tiff( tif );

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
    {"validate_tiff_from_buffer",  py_validate_tiff_from_buffer, METH_VARARGS, validate_tiff_from_buffer__doc__},
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
