#include <string.h>
#include <stdio.h>

#define ANY_JSON_IMPLEMENT
#include "any_json.h"

// TODO
void test_read()
{
    const char *src =
        "{\n"
        "\t\"hello\": 1,\n"
        "\t\"key2\": true,\n"
        "\t\"_aaa__\": { },\n"
        "\t\"dict\": [1, 2, { \"key\": \"value\" }],\n"
        "\t\"varx\": 1.1\n"
        "}\n";

    //any_json_reader_t json;
    //any_json_reader_init(&json, src, strlen(src));

    //while ((key = any_json_reader_key(&json)) != NULL) {
    //    switch (any_json_reader_type(&json)) {
    //        case ANY_JSON_STRING:
    //            break;

    //        case ANY_JSON_NUMBER:
    //            break;

    //        case ANY_JSON_BOOL:
    //            break;

    //        case ANY_JSON_NULL:
    //            break;
    //    }
    //}

    //if (any_json_next_object(&json)) {
    //    char *key;
    //    while ((key = any_json_next_key(&json)) != NULL) {
    //        printf("KEY: %s\n", key);

    //        any_json_
    //    }
    //    any_json_end(&json);
    //}
}

void test_write(bool pretty)
{
    any_json_write_t json;
    any_json_write_init(&json, stdout, (any_json_write_fputs_t)fputs, pretty);

    any_json_write_open_object(&json);

    const char *props[][2] = {
        { "name", "test" },
        { "scope", "local" },
        { "project", "any_json" },
        { "author", "me" },
        { 0 },
    };

    for (int i = 0; props[i][0] != NULL; i++) {
        any_json_write_member(&json, props[i][0]);
        any_json_write_string(&json, props[i][1]);
    }

    any_json_write_member(&json, "list");
    any_json_write_open_array(&json);
    {
        any_json_write_member(&json, NULL);
        any_json_write_number(&json, 1111111);

        any_json_write_member(&json, NULL);
        any_json_write_number(&json, 49e32);

        any_json_write_member(&json, NULL);
        any_json_write_number(&json, 99.99);

        any_json_write_member(&json, NULL);
        any_json_write_open_array(&json);
        {
            any_json_write_member(&json, NULL);
            any_json_write_string(&json, "nested");

            any_json_write_member(&json, NULL);
            any_json_write_string(&json, "array");
        }
        any_json_write_close_array(&json);
    }
    any_json_write_close_array(&json);

    any_json_write_member(&json, "sub");
    any_json_write_open_object(&json);
    {
        any_json_write_member(&json, "data");
        any_json_write_open_object(&json);
        {
            any_json_write_member(&json, "handle");
            any_json_write_null(&json);

            any_json_write_member(&json, "version");
            any_json_write_number(&json, 1.1);
        }
        any_json_write_close_object(&json);

        any_json_write_member(&json, "pack");
        any_json_write_open_array(&json);
        {
            any_json_write_member(&json, NULL);
            any_json_write_string(&json, "bin");

            any_json_write_member(&json, NULL);
            any_json_write_string(&json, "lib");

            any_json_write_member(&json, NULL);
            any_json_write_bool(&json, true);
        }
        any_json_write_close_array(&json);
    }
    any_json_write_close_object(&json);

    any_json_write_close_object(&json);

    puts("\n");
}

int main()
{
    printf("JSON WRITE TEST\n");
    test_write(false);

    printf("JSON PRETTY WRITE TEST\n");
    test_write(true);

    return 0;
}

