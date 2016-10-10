#include <Python.h>
#include "segyio/segy.h"
#include <assert.h>

// ---------------  FILE Handling ------------
static FILE *get_FILE_pointer_from_capsule(PyObject *capsule) {
    if (!PyCapsule_IsValid(capsule, "FILE*")) {
        PyErr_SetString(PyExc_TypeError, "The object was not of type FILE");
        return NULL;
    }

    if(PyCapsule_GetDestructor(capsule) == NULL) {
        PyErr_SetString(PyExc_IOError, "The file has already been closed");
        return NULL;
    }

    FILE *p_FILE = PyCapsule_GetPointer(capsule, "FILE*");

    if (!p_FILE) {
        PyErr_SetString(PyExc_ValueError, "File Handle is NULL");
        return NULL;
    }
    return p_FILE;
}

static void *py_FILE_destructor(PyObject *capsule) {
#ifndef NDEBUG
    fputs("FILE* destructed before calling close()\n", stderr);
#endif
    return NULL;
}

static PyObject *py_FILE_open(PyObject *self, PyObject *args) {
    char *filename = NULL;
    char *mode = NULL;
    PyArg_ParseTuple(args, "ss", &filename, &mode);

    FILE *p_FILE = fopen(filename, mode);

    if (p_FILE == NULL) {
        return PyErr_SetFromErrnoWithFilename(PyExc_IOError, filename);
    }
    return PyCapsule_New(p_FILE, "FILE*", (PyCapsule_Destructor) py_FILE_destructor);
}

static PyObject *py_FILE_close(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    PyArg_ParseTuple(args, "O", &file_capsule);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    if (PyErr_Occurred()) { return NULL; }

    fclose(p_FILE);

    if (errno != 0) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    PyCapsule_SetDestructor(file_capsule, NULL);

    return Py_BuildValue("");
}

static PyObject *py_FILE_flush(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    PyArg_ParseTuple(args, "O", &file_capsule);

    FILE *p_FILE = NULL;
    if(file_capsule != Py_None) {
        p_FILE = get_FILE_pointer_from_capsule(file_capsule);
    }

    fflush(p_FILE);

    if (errno != 0) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    return Py_BuildValue("");
}


// ------------- ERROR Handling -------------
struct error_args {
    int error;
    int errno_err;
    int field_1;
    int field_2;
    int field_count;
    const char *name;
};

static PyObject *py_handle_segy_error_(struct error_args args) {
    switch (args.error) {
        case SEGY_TRACE_SIZE_MISMATCH:
            return PyErr_Format(PyExc_RuntimeError,
                                "Number of traces is not consistent with file size. File may be corrupt.");

        case SEGY_INVALID_FIELD:
            if (args.field_count == 1) {
                return PyErr_Format(PyExc_IndexError, "Field value out of range: %d", args.field_1);
            } else {
                int inline_field = args.field_1;
                int crossline_field = args.field_2;
                return PyErr_Format(PyExc_IndexError, "Invalid inline (%d) or crossline (%d) field/byte offset. "
                        "Too large or between valid byte offsets.", inline_field, crossline_field);
            }
        case SEGY_INVALID_OFFSETS:
            return PyErr_Format(PyExc_RuntimeError, "Found more offsets than traces. File may be corrupt.");

        case SEGY_INVALID_SORTING:
            return PyErr_Format(PyExc_RuntimeError, "Unable to determine sorting. File may be corrupt.");

        case SEGY_INVALID_ARGS:
            return PyErr_Format(PyExc_RuntimeError, "Input arguments are invalid.");

        case SEGY_MISSING_LINE_INDEX:
            return PyErr_Format(PyExc_KeyError, "%s number %d does not exist.", args.name, args.field_1);

        default:
            errno = args.errno_err;
            return PyErr_SetFromErrno(PyExc_IOError);
    }
}

static PyObject *py_handle_segy_error(int error, int errno_err) {
    struct error_args args;
    args.error = error;
    args.errno_err = errno_err;
    args.field_1 = 0;
    args.field_2 = 0;
    args.field_count = 0;
    args.name = "";
    return py_handle_segy_error_(args);
}

static PyObject *py_handle_segy_error_with_fields(int error, int errno_err, int field_1, int field_2, int field_count) {
    struct error_args args;
    args.error = error;
    args.errno_err = errno_err;
    args.field_1 = field_1;
    args.field_2 = field_2;
    args.field_count = field_count;
    args.name = "";
    return py_handle_segy_error_(args);
}

static PyObject *py_handle_segy_error_with_index_and_name(int error, int errno_err, int index, const char *name) {
    struct error_args args;
    args.error = error;
    args.errno_err = errno_err;
    args.field_1 = index;
    args.field_2 = 0;
    args.field_count = 1;
    args.name = name;
    return py_handle_segy_error_(args);
}

// ------------ Text Header -------------

static PyObject *py_textheader_size(PyObject *self) {
    return Py_BuildValue("i", segy_textheader_size());
}

static PyObject *py_read_texthdr(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    int index;

    PyArg_ParseTuple(args, "Oi", &file_capsule, &index);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    char *buffer = malloc(sizeof(char) * (segy_textheader_size()));

    int error = segy_read_textheader(p_FILE, buffer);

    if (error != 0) {
        free(buffer);
        return PyErr_Format(PyExc_Exception, "Could not read text header: %s", strerror(errno));
    }

    PyObject *result = Py_BuildValue("s", buffer);
    free(buffer);
    return result;
}

static PyObject *py_write_texthdr(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    unsigned int index;
    char *buffer;
    int size;

    PyArg_ParseTuple(args, "Ois#", &file_capsule, &index, &buffer, &size);

    if (size < segy_textheader_size() - 1) {
        return PyErr_Format(PyExc_ValueError, "String must have at least 3200 characters. Received count: %d", size);
    }

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    int error = segy_write_textheader(p_FILE, index, buffer);

    if (error == 0) {
        return Py_BuildValue("");
    } else {
        return py_handle_segy_error(error, errno);
    }
}

// ------------ Binary and Trace Header ------------
static char *get_header_pointer_from_capsule(PyObject *capsule, unsigned int *length) {
    if (PyCapsule_IsValid(capsule, "BinaryHeader=char*")) {
        if (length) {
            *length = segy_binheader_size();
        }
        return PyCapsule_GetPointer(capsule, "BinaryHeader=char*");

    } else if (PyCapsule_IsValid(capsule, "TraceHeader=char*")) {
        if (length) {
            *length = SEGY_TRACE_HEADER_SIZE;
        }
        return PyCapsule_GetPointer(capsule, "TraceHeader=char*");
    }
    PyErr_SetString(PyExc_TypeError, "The object was not a header type");
    return NULL;
}


static PyObject *py_get_field(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *header_capsule = NULL;
    int field;

    PyArg_ParseTuple(args, "Oi", &header_capsule, &field);

    unsigned int length;
    char *header = get_header_pointer_from_capsule(header_capsule, &length);

    if (PyErr_Occurred()) { return NULL; }

    int value;
    int error;
    if (length == segy_binheader_size()) {
        error = segy_get_bfield(header, field, &value);
    } else {
        error = segy_get_field(header, field, &value);
    }

    if (error == 0) {
        return Py_BuildValue("i", value);
    } else {
        return py_handle_segy_error_with_fields(error, errno, field, 0, 1);
    }
}

static PyObject *py_set_field(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *header_capsule = NULL;
    int field;
    int value;

    PyArg_ParseTuple(args, "Oii", &header_capsule, &field, &value);

    unsigned int length;
    char *header = get_header_pointer_from_capsule(header_capsule, &length);

    if (PyErr_Occurred()) { return NULL; }

    int error;
    if (length == segy_binheader_size()) {
        error = segy_set_bfield(header, field, value);
    } else {
        error = segy_set_field(header, field, value);
    }

    if (error == 0) {
        return Py_BuildValue("");
    } else {
        return py_handle_segy_error_with_fields(error, errno, field, 0, 1);
    }
}

// ------------ Binary Header -------------
static PyObject *py_binheader_size(PyObject *self) {
    return Py_BuildValue("i", segy_binheader_size());
}

static void *py_binary_header_destructor(PyObject *capsule) {
    char *binary_header = get_header_pointer_from_capsule(capsule, NULL);
    free(binary_header);
    return NULL;
}

static PyObject *py_empty_binaryhdr(PyObject *self) {
    errno = 0;
    char *buffer = calloc(segy_binheader_size(), sizeof(char));
    return PyCapsule_New(buffer, "BinaryHeader=char*", (PyCapsule_Destructor) py_binary_header_destructor);
}

static PyObject *py_read_binaryhdr(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;

    PyArg_ParseTuple(args, "O", &file_capsule);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    if (PyErr_Occurred()) { return NULL; }

    char *buffer = malloc(sizeof(char) * (segy_binheader_size()));

    int error = segy_binheader(p_FILE, buffer);

    if (error == 0) {
        return PyCapsule_New(buffer, "BinaryHeader=char*", (PyCapsule_Destructor) py_binary_header_destructor);
    } else {
        free(buffer);
        return py_handle_segy_error(error, errno);
    }
}

static PyObject *py_write_binaryhdr(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    PyObject *binary_header_capsule = NULL;

    PyArg_ParseTuple(args, "OO", &file_capsule, &binary_header_capsule);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);
    char *binary_header = get_header_pointer_from_capsule(binary_header_capsule, NULL);

    if (PyErr_Occurred()) { return NULL; }

    int error = segy_write_binheader(p_FILE, binary_header);

    if (error == 0) {
        return Py_BuildValue("");
    } else {
        return py_handle_segy_error(error, errno);
    }
}

// -------------- Trace Headers ----------
static char *get_trace_header_pointer_from_capsule(PyObject *capsule) {
    if (!PyCapsule_IsValid(capsule, "TraceHeader=char*")) {
        PyErr_Format(PyExc_TypeError, "The object was not of type TraceHeader.");
        return NULL;
    }
    return PyCapsule_GetPointer(capsule, "TraceHeader=char*");
}

static void *py_trace_header_destructor(PyObject *capsule) {
    char *trace_header = get_trace_header_pointer_from_capsule(capsule);
    free(trace_header);
    return NULL;
}

static PyObject *py_empty_trace_header(PyObject *self) {
    errno = 0;
    char *buffer = calloc(SEGY_TRACE_HEADER_SIZE, sizeof(char));
    return PyCapsule_New(buffer, "TraceHeader=char*", (PyCapsule_Destructor) py_trace_header_destructor);
}

static PyObject *py_read_trace_header(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    unsigned int traceno;
    PyObject *trace_header_capsule = NULL;
    long trace0;
    unsigned int trace_bsize;

    PyArg_ParseTuple(args, "OIOlI", &file_capsule, &traceno, &trace_header_capsule, &trace0, &trace_bsize);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    if (PyErr_Occurred()) { return NULL; }

    char *buffer = get_trace_header_pointer_from_capsule(trace_header_capsule);

    if (PyErr_Occurred()) { return NULL; }

    int error = segy_traceheader(p_FILE, traceno, buffer, trace0, trace_bsize);

    if (error == 0) {
        Py_IncRef(trace_header_capsule);
        return trace_header_capsule;
    } else {
        return py_handle_segy_error(error, errno);
    }
}

static PyObject *py_write_trace_header(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    unsigned int traceno;
    PyObject *trace_header_capsule = NULL;
    long trace0;
    unsigned int trace_bsize;

    PyArg_ParseTuple(args, "OIOlI", &file_capsule, &traceno, &trace_header_capsule, &trace0, &trace_bsize);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    if (PyErr_Occurred()) { return NULL; }

    char *buffer = get_trace_header_pointer_from_capsule(trace_header_capsule);

    if (PyErr_Occurred()) { return NULL; }

    int error = segy_write_traceheader(p_FILE, traceno, buffer, trace0, trace_bsize);

    if (error == 0) {
        return Py_BuildValue("");
    } else {
        return py_handle_segy_error(error, errno);
    }
}

static PyObject *py_trace_bsize(PyObject *self, PyObject *args) {
    errno = 0;
    unsigned int sample_count;

    PyArg_ParseTuple(args, "I", &sample_count);

    unsigned int byte_count = segy_trace_bsize(sample_count);

    return Py_BuildValue("I", byte_count);
}


static PyObject *py_init_line_metrics(PyObject *self, PyObject *args) {
    errno = 0;
    SEGY_SORTING sorting;
    unsigned int trace_count;
    unsigned int inline_count;
    unsigned int crossline_count;
    unsigned int offset_count;

    PyArg_ParseTuple(args, "iIIII", &sorting, &trace_count, &inline_count, &crossline_count, &offset_count);

    unsigned int iline_length;
    int error = segy_inline_length(sorting, trace_count, crossline_count, offset_count, &iline_length);

    //Only check first call since the only error that can occur is SEGY_INVALID_SORTING
    if (error != 0) {
        return py_handle_segy_error(error, errno);
    }

    unsigned int xline_length;
    segy_crossline_length(sorting, trace_count, inline_count, offset_count, &xline_length);

    unsigned int iline_stride;
    segy_inline_stride(sorting, inline_count, &iline_stride);

    unsigned int xline_stride;
    segy_crossline_stride(sorting, crossline_count, &xline_stride);

    PyObject *dict = PyDict_New();
    PyDict_SetItemString(dict, "xline_length", Py_BuildValue("I", xline_length));
    PyDict_SetItemString(dict, "xline_stride", Py_BuildValue("I", xline_stride));
    PyDict_SetItemString(dict, "iline_length", Py_BuildValue("I", iline_length));
    PyDict_SetItemString(dict, "iline_stride", Py_BuildValue("I", iline_stride));

    return Py_BuildValue("O", dict);
}


static PyObject *py_init_metrics(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    PyObject *binary_header_capsule = NULL;
    int il_field;
    int xl_field;

    PyArg_ParseTuple(args, "OOii", &file_capsule, &binary_header_capsule, &il_field, &xl_field);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    if (PyErr_Occurred()) { return NULL; }

    char *binary_header = get_header_pointer_from_capsule(binary_header_capsule, NULL);

    if (PyErr_Occurred()) { return NULL; }

    long trace0 = segy_trace0(binary_header);
    unsigned int sample_count = segy_samples(binary_header);
    int format = segy_format(binary_header);
    unsigned int trace_bsize = segy_trace_bsize(sample_count);

    int sorting;
    int error = segy_sorting(p_FILE, il_field, xl_field, &sorting, trace0, trace_bsize);

    if (error != 0) {
        return py_handle_segy_error_with_fields(error, errno, il_field, xl_field, 2);
    }

    size_t trace_count;
    error = segy_traces(p_FILE, &trace_count, trace0, trace_bsize);

    if (error != 0) {
        return py_handle_segy_error(error, errno);
    }

    unsigned int offset_count;
    error = segy_offsets(p_FILE, il_field, xl_field, (unsigned int) trace_count, &offset_count, trace0, trace_bsize);

    if (error != 0) {
        return py_handle_segy_error_with_fields(error, errno, il_field, xl_field, 2);
    }

    int field;
    unsigned int xl_count;
    unsigned int il_count;
    unsigned int *l1out;
    unsigned int *l2out;

    if (sorting == CROSSLINE_SORTING) {
        field = il_field;
        l1out = &xl_count;
        l2out = &il_count;
    } else if (sorting == INLINE_SORTING) {
        field = xl_field;
        l1out = &il_count;
        l2out = &xl_count;
    } else {
        return PyErr_Format(PyExc_RuntimeError, "Unable to determine sorting. File may be corrupt.");
    }

    error = segy_count_lines(p_FILE, field, offset_count, l1out, l2out, trace0, trace_bsize);

    if (error != 0) {
        return py_handle_segy_error_with_fields(error, errno, il_field, xl_field, 2);
    }

    PyObject *dict = PyDict_New();
    PyDict_SetItemString(dict, "iline_field", Py_BuildValue("i", il_field));
    PyDict_SetItemString(dict, "xline_field", Py_BuildValue("i", xl_field));
    PyDict_SetItemString(dict, "trace0", Py_BuildValue("l", trace0));
    PyDict_SetItemString(dict, "sample_count", Py_BuildValue("I", sample_count));
    PyDict_SetItemString(dict, "format", Py_BuildValue("i", format));
    PyDict_SetItemString(dict, "trace_bsize", Py_BuildValue("I", trace_bsize));
    PyDict_SetItemString(dict, "sorting", Py_BuildValue("i", sorting));
    PyDict_SetItemString(dict, "trace_count", Py_BuildValue("k", trace_count));
    PyDict_SetItemString(dict, "offset_count", Py_BuildValue("I", offset_count));
    PyDict_SetItemString(dict, "iline_count", Py_BuildValue("I", il_count));
    PyDict_SetItemString(dict, "xline_count", Py_BuildValue("I", xl_count));

    return Py_BuildValue("O", dict);
}

static Py_buffer check_and_get_buffer(PyObject *object, const char *name, unsigned int expected) {
    Py_buffer buffer;
    if (!PyObject_CheckBuffer(object)) {
        PyErr_Format(PyExc_TypeError, "The destination for %s is not a buffer object", name);
        return buffer;
    }
    PyObject_GetBuffer(object, &buffer, PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITEABLE);

    if (strcmp(buffer.format, "I") != 0) {
        PyErr_Format(PyExc_TypeError, "The destination for %s is not a buffer object of type 'uintc'", name);
        PyBuffer_Release(&buffer);
        return buffer;
    }

    if (buffer.len < expected * sizeof(unsigned int)) {
        PyErr_Format(PyExc_ValueError, "The destination for %s is too small. ", name);
        PyBuffer_Release(&buffer);
        return buffer;
    }

    return buffer;
}


static PyObject *py_init_line_indices(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    PyObject *metrics = NULL;
    PyObject *iline_out = NULL;
    PyObject *xline_out = NULL;

    PyArg_ParseTuple(args, "OOOO", &file_capsule, &metrics, &iline_out, &xline_out);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    if (PyErr_Occurred()) { return NULL; }

    if (!PyDict_Check(metrics)) {
        PyErr_SetString(PyExc_TypeError, "metrics is not a dictionary!");
        return NULL;
    }

    unsigned int iline_count;
    unsigned int xline_count;
    PyArg_Parse(PyDict_GetItemString(metrics, "iline_count"), "I", &iline_count);
    PyArg_Parse(PyDict_GetItemString(metrics, "xline_count"), "I", &xline_count);

    if (PyErr_Occurred()) { return NULL; }

    Py_buffer iline_buffer = check_and_get_buffer(iline_out, "inline", iline_count);

    if (PyErr_Occurred()) { return NULL; }

    Py_buffer xline_buffer = check_and_get_buffer(xline_out, "crossline", xline_count);

    if (PyErr_Occurred()) {
        PyBuffer_Release(&iline_buffer);
        return NULL;
    }

    int il_field;
    int xl_field;
    int sorting;
    unsigned int offset_count;
    long trace0;
    unsigned int trace_bsize;

    PyArg_Parse(PyDict_GetItemString(metrics, "iline_field"), "i", &il_field);
    PyArg_Parse(PyDict_GetItemString(metrics, "xline_field"), "i", &xl_field);
    PyArg_Parse(PyDict_GetItemString(metrics, "sorting"), "i", &sorting);
    PyArg_Parse(PyDict_GetItemString(metrics, "offset_count"), "I", &offset_count);
    PyArg_Parse(PyDict_GetItemString(metrics, "trace0"), "l", &trace0);
    PyArg_Parse(PyDict_GetItemString(metrics, "trace_bsize"), "I", &trace_bsize);

    int error = segy_inline_indices(p_FILE, il_field, sorting, iline_count, xline_count, offset_count, iline_buffer.buf,
                                    trace0, trace_bsize);

    if (error != 0) {
        py_handle_segy_error_with_fields(error, errno, il_field, xl_field, 2);
    }

    error = segy_crossline_indices(p_FILE, xl_field, sorting, iline_count, xline_count, offset_count, xline_buffer.buf,
                                   trace0, trace_bsize);

    if (error != 0) {
        py_handle_segy_error_with_fields(error, errno, il_field, xl_field, 2);
    }

    PyBuffer_Release(&xline_buffer);
    PyBuffer_Release(&iline_buffer);
    return Py_BuildValue("");
}


static PyObject *py_fread_trace0(PyObject *self, PyObject *args) {
    errno = 0;
    unsigned int lineno;
    unsigned int other_line_length;
    unsigned int stride;
    PyObject *indices_object;
    char *type_name;

    PyArg_ParseTuple(args, "IIIOs", &lineno, &other_line_length, &stride, &indices_object, &type_name);

    Py_buffer buffer;
    if (!PyObject_CheckBuffer(indices_object)) {
        PyErr_Format(PyExc_TypeError, "The destination for %s is not a buffer object", type_name);
        return NULL;
    }
    PyObject_GetBuffer(indices_object, &buffer, PyBUF_FORMAT | PyBUF_C_CONTIGUOUS);

    unsigned int trace_no;
    unsigned int linenos_sz = (unsigned int) PyObject_Length(indices_object);
    int error = segy_line_trace0(lineno, other_line_length, stride, buffer.buf, linenos_sz, &trace_no);

    if (error != 0) {
        return py_handle_segy_error_with_index_and_name(error, errno, lineno, type_name);
    }

    return Py_BuildValue("I", trace_no);
}

static PyObject *py_read_trace(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    unsigned int trace_no;
    PyObject *buffer_out;
    long trace0;
    unsigned int trace_bsize;
    int format;
    unsigned int samples;

    PyArg_ParseTuple(args, "OIOlIiI", &file_capsule, &trace_no, &buffer_out, &trace0, &trace_bsize, &format, &samples);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    if (PyErr_Occurred()) { return NULL; }

    if (!PyObject_CheckBuffer(buffer_out)) {
        PyErr_SetString(PyExc_TypeError, "The destination buffer is not of the correct type.");
        return NULL;
    }
    Py_buffer buffer;
    PyObject_GetBuffer(buffer_out, &buffer, PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITEABLE);

    int error = segy_readtrace(p_FILE, trace_no, buffer.buf, trace0, trace_bsize);

    if (error != 0) {
        return py_handle_segy_error_with_index_and_name(error, errno, trace_no, "Trace");
    }

    error = segy_to_native(format, samples, buffer.buf);

    if (error != 0) {
        PyErr_SetString(PyExc_TypeError, "Unable to convert buffer to native format.");
        return NULL;
    }

    Py_IncRef(buffer_out);
    return buffer_out;
}

static PyObject *py_write_trace(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    unsigned int trace_no;
    PyObject *buffer_in;
    long trace0;
    unsigned int trace_bsize;
    int format;
    unsigned int samples;

    PyArg_ParseTuple(args, "OIOlIiI", &file_capsule, &trace_no, &buffer_in, &trace0, &trace_bsize, &format, &samples);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    if (PyErr_Occurred()) { return NULL; }

    if (!PyObject_CheckBuffer(buffer_in)) {
        PyErr_SetString(PyExc_TypeError, "The source buffer is not of the correct type.");
        return NULL;
    }
    Py_buffer buffer;
    PyObject_GetBuffer(buffer_in, &buffer, PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITEABLE);

    int error = segy_from_native(format, samples, buffer.buf);

    if (error != 0) {
        PyErr_SetString(PyExc_TypeError, "Unable to convert buffer from native format.");
        return NULL;
    }

    error = segy_writetrace(p_FILE, trace_no, buffer.buf, trace0, trace_bsize);

    if (error != 0) {
        return py_handle_segy_error_with_index_and_name(error, errno, trace_no, "Trace");
    }

    error = segy_to_native(format, samples, buffer.buf);

    if (error != 0) {
        PyErr_SetString(PyExc_TypeError, "Unable to convert buffer to native format.");
        return NULL;
    }

    Py_IncRef(buffer_in);
    return buffer_in;
}

static PyObject *py_read_line(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *file_capsule = NULL;
    unsigned int line_trace0;
    unsigned int line_length;
    unsigned int stride;
    PyObject *buffer_in;
    long trace0;
    unsigned int trace_bsize;
    int format;
    unsigned int samples;

    PyArg_ParseTuple(args, "OIIIOlIiI", &file_capsule, &line_trace0, &line_length, &stride, &buffer_in, &trace0,
                     &trace_bsize, &format, &samples);

    FILE *p_FILE = get_FILE_pointer_from_capsule(file_capsule);

    if (PyErr_Occurred()) { return NULL; }

    if (!PyObject_CheckBuffer(buffer_in)) {
        PyErr_SetString(PyExc_TypeError, "The destination buffer is not of the correct type.");
        return NULL;
    }
    Py_buffer buffer;
    PyObject_GetBuffer(buffer_in, &buffer, PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITEABLE);

    int error = segy_read_line(p_FILE, line_trace0, line_length, stride, buffer.buf, trace0, trace_bsize);

    if (error != 0) {
        return py_handle_segy_error_with_index_and_name(error, errno, line_trace0, "Line");
    }

    error = segy_to_native(format, samples * line_length, buffer.buf);

    if (error != 0) {
        PyErr_SetString(PyExc_TypeError, "Unable to convert buffer to native format.");
        return NULL;
    }

    Py_IncRef(buffer_in);
    return buffer_in;
}

/*  define functions in module */
static PyMethodDef SegyMethods[] = {
        {"open",               (PyCFunction) py_FILE_open,          METH_VARARGS, "Opens a file."},
        {"close",              (PyCFunction) py_FILE_close,         METH_VARARGS, "Closes a file."},
        {"flush",              (PyCFunction) py_FILE_flush,         METH_VARARGS, "Flushes a file."},

        {"binheader_size",     (PyCFunction) py_binheader_size,     METH_NOARGS,  "Return the size of the binary header."},
        {"textheader_size",    (PyCFunction) py_textheader_size,    METH_NOARGS,  "Return the size of the text header."},

        {"read_textheader",    (PyCFunction) py_read_texthdr,       METH_VARARGS, "Reads the text header from a segy file."},
        {"write_textheader",   (PyCFunction) py_write_texthdr,      METH_VARARGS, "Write the text header to a segy file."},

        {"empty_binaryheader", (PyCFunction) py_empty_binaryhdr,    METH_NOARGS,  "Create empty binary header for a segy file."},
        {"read_binaryheader",  (PyCFunction) py_read_binaryhdr,     METH_VARARGS, "Read the binary header from a segy file."},
        {"write_binaryheader", (PyCFunction) py_write_binaryhdr,    METH_VARARGS, "Write the binary header to a segy file."},

        {"empty_traceheader",  (PyCFunction) py_empty_trace_header, METH_NOARGS,  "Create empty trace header for a segy file."},
        {"read_traceheader",   (PyCFunction) py_read_trace_header,  METH_VARARGS, "Read a trace header from a segy file."},
        {"write_traceheader",  (PyCFunction) py_write_trace_header, METH_VARARGS, "Write a trace header to a segy file."},

        {"trace_bsize",        (PyCFunction) py_trace_bsize,        METH_VARARGS, "Returns the number of bytes in a trace."},
        {"get_field",          (PyCFunction) py_get_field,          METH_VARARGS, "Get a header field."},
        {"set_field",          (PyCFunction) py_set_field,          METH_VARARGS, "Set a header field."},

        {"init_line_metrics",  (PyCFunction) py_init_line_metrics,  METH_VARARGS, "Find the length and stride of inline and crossline."},
        {"init_metrics",       (PyCFunction) py_init_metrics,       METH_VARARGS, "Find most metrics for a segy file."},
        {"init_line_indices",  (PyCFunction) py_init_line_indices,  METH_VARARGS, "Find the indices for inline and crossline."},
        {"fread_trace0",       (PyCFunction) py_fread_trace0,       METH_VARARGS, "Find trace0 of a line."},
        {"read_trace",         (PyCFunction) py_read_trace,         METH_VARARGS, "Read trace data."},
        {"write_trace",        (PyCFunction) py_write_trace,        METH_VARARGS, "Write trace data."},
        {"read_line",          (PyCFunction) py_read_line,          METH_VARARGS, "Read a xline/iline from file."},
        {NULL, NULL, 0, NULL}
};


/* module initialization */
PyMODINIT_FUNC
init_segyio(void) {
    (void) Py_InitModule("_segyio", SegyMethods);
}