#include <HalonMTA.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <string.h>
#include <zip.h>

struct zip_wrapper
{
	zip_t* archive;
	zip_source_t* source;
};

void ZIP_class_addFile(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	if (HalonMTA_hsl_argument_length(args) != 2)
	{
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, "Invalid number of parameters", 0);
		return;
	}

	HalonHSLValue* a = HalonMTA_hsl_argument_get(args, 0);
	char* name = nullptr;
	size_t namelen;
	if (!a || HalonMTA_hsl_value_type(a) != HALONMTA_HSL_TYPE_STRING ||
			!HalonMTA_hsl_value_get(a, HALONMTA_HSL_TYPE_STRING, &name, &namelen))
	{
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, "Invalid type of name parameter", 0);
		return;
	}

	a = HalonMTA_hsl_argument_get(args, 1);
	char* data = nullptr;
	size_t datalen;
	if (!a || HalonMTA_hsl_value_type(a) != HALONMTA_HSL_TYPE_STRING ||
			!HalonMTA_hsl_value_get(a, HALONMTA_HSL_TYPE_STRING, &data, &datalen))
	{
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, "Invalid type of data parameter", 0);
		return;
	}

	zip_wrapper* zip = (zip_wrapper*)HalonMTA_hsl_object_ptr_get(hhc);

	if (zip->archive == nullptr)
	{
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, "No open archive", 0);
		return;
	}

	void* copy = malloc(datalen);
	if (!copy)
	{
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, "Out of memory", 0);
		return;
	}
	memcpy(copy, data, datalen);

	zip_source_t *source = zip_source_buffer(zip->archive, copy, datalen, 1);
	if (!source)
	{
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, zip_strerror(zip->archive), 0);
		return;
	}

	zip_int64_t idx = zip_file_add(zip->archive, name, source, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE);
	if (idx < 0)
	{
		zip_source_free(source); /* refcounted: free on error */	
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, zip_strerror(zip->archive), 0);
		return;
	}

	HalonMTA_hsl_value_set(ret, HALONMTA_HSL_TYPE_THIS, hhc, 0);
}

void ZIP_class_toString(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	char* password = nullptr;

	if (HalonMTA_hsl_argument_length(args) > 1)
	{
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, "Invalid number of parameters", 0);
		return;
	}
	if (HalonMTA_hsl_argument_length(args) > 0)
	{
		HalonHSLValue* k = HalonMTA_hsl_argument_get(args, 0);
		HalonHSLValue* o = HalonMTA_hsl_value_array_find(k, "password");
		if (o)
		{
			if (HalonMTA_hsl_value_type(o) != HALONMTA_HSL_TYPE_STRING ||
					!HalonMTA_hsl_value_get(o, HALONMTA_HSL_TYPE_STRING, &password, nullptr))
			{
				HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
				HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, "Invalid password", 0);
				return;
			}
		}
	}

	zip_wrapper* zip = (zip_wrapper*)HalonMTA_hsl_object_ptr_get(hhc);

	if (zip->archive == nullptr)
	{
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, "No open archive", 0);
		return;
	}

	if (password)
	{
		zip_int64_t e = zip_get_num_entries(zip->archive, 0);
		for (zip_uint64_t i = 0; i < (zip_uint64_t)e; i++)
		{
			if (zip_file_set_encryption(zip->archive, i, ZIP_EM_AES_256, password) < 0)
			{
				HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
				HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, zip_strerror(zip->archive), 0);
				return;
			}
		}
	}

    if (zip_close(zip->archive) < 0)
	{
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, zip_strerror(zip->archive), 0);

		zip_discard(zip->archive);
		zip->archive = nullptr;
		return;
    }

	zip->archive = nullptr;

	void *zip_bytes = NULL;
    zip_uint64_t zip_size = 0;

    if (zip_source_is_deleted(zip->source))
	{
        /* archive ended up empty */
        zip_bytes = NULL;
        zip_size = 0;
    }
	else
	{
        zip_stat_t st;
        zip_stat_init(&st);

        if (zip_source_stat(zip->source, &st) < 0)
		{
			HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, zip_error_strerror(zip_source_error(zip->source)), 0);
		    return;
        }

        zip_size = st.size;
        zip_bytes = malloc((size_t)zip_size);
        if (zip_bytes == NULL)
		{
			HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, "Could not export ZIP", 0);
            return;
        }

        if (zip_source_open(zip->source) < 0)
		{
            free(zip_bytes);

			HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, zip_error_strerror(zip_source_error(zip->source)), 0);
		    return;
        }

        zip_int64_t n = zip_source_read(zip->source, zip_bytes, zip_size);
        zip_source_close(zip->source);

        if (n < 0 || (zip_uint64_t)n != zip_size)
		{
            free(zip_bytes);

			HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, zip_error_strerror(zip_source_error(zip->source)), 0);
		    return;
        }
    }

	HalonMTA_hsl_value_set(ret, HALONMTA_HSL_TYPE_STRING, zip_bytes, zip_size);
    free(zip_bytes);
}

void ZIP_object_free(void* ptr)
{
	zip_wrapper* zip = (zip_wrapper*)ptr;
	if (zip->archive)
		zip_discard(zip->archive);
	zip_source_free(zip->source);
	free(zip);
}

HALON_EXPORT
void ZIP_class(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	if (HalonMTA_hsl_argument_length(args) > 0)
	{
		HalonHSLValue* x = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(x, HALONMTA_HSL_TYPE_EXCEPTION, "Invalid number of parameters", 0);
		return;
	}

    zip_error_t error;
    zip_source_t *src;
    zip_t *za;

    /* replace these with actual ZIP bytes */
    const unsigned char *zip_data = nullptr;
    zip_uint64_t zip_size = 0;

    zip_error_init(&error);

    src = zip_source_buffer_create(zip_data, zip_size, 0, &error);
    if (src == NULL) {
        fprintf(stderr, "zip_source_buffer_create: %s\n",
                zip_error_strerror(&error));
        zip_error_fini(&error);
        return;
    }

    za = zip_open_from_source(src, ZIP_CREATE, &error);

    if (za == NULL) {
        fprintf(stderr, "zip_open_from_source: %s\n",
                zip_error_strerror(&error));
        zip_source_free(src);   /* only on error */
        zip_error_fini(&error);
        return;
    }


    zip_source_keep(src);

	zip_wrapper* ptr = (zip_wrapper*)malloc(sizeof(zip_wrapper));
	ptr->archive = za;
	ptr->source = src;

	HalonHSLObject* object = HalonMTA_hsl_object_new();
	HalonMTA_hsl_object_type_set(object, "ZIP");
	HalonMTA_hsl_object_register_function(object, "addFile", &ZIP_class_addFile);
	HalonMTA_hsl_object_register_function(object, "toString", &ZIP_class_toString);
	HalonMTA_hsl_object_ptr_set(object, ptr, ZIP_object_free);
	HalonMTA_hsl_value_set(ret, HALONMTA_HSL_TYPE_OBJECT, object, 0);
	HalonMTA_hsl_object_delete(object);
}

HALON_EXPORT
bool Halon_hsl_register(HalonHSLRegisterContext* ptr)
{
	HalonMTA_hsl_module_register_function(ptr, "ZIP", &ZIP_class);
	return true;
}

HALON_EXPORT
int Halon_version()
{
	return HALONMTA_PLUGIN_VERSION;
}