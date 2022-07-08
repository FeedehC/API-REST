#include <ulfius.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8081 // El puerto del contador

// Callback function for the web application on /helloworld url call
int callback_hello_world(const struct _u_request *request, struct _u_response *response, void *user_data)
{
    (void)request;
    (void)user_data;
    json_t *hello = json_object();
    json_object_set_new(hello, "text", json_string("Hello World!"));
    ulfius_set_json_body_response(response, 200, hello);
    return U_CALLBACK_CONTINUE;
}

// Callback function for the web application on /print url call
int callback_value(const struct _u_request *request, struct _u_response *response, void *user_data)
{
    (void)request;
    int cnt = *((int *)user_data);
    const unsigned int code = 200;
    json_t *counter = json_object();
    json_object_set_new(counter, "code", json_integer(code));
    json_object_set_new(counter, "description", json_integer(cnt));
    ulfius_set_json_body_response(response, code, counter);
    return U_CALLBACK_CONTINUE;
}

// Callback function for the web application on /increment url call
int callback_increment(const struct _u_request *request, struct _u_response *response, void *user_data)
{
    (void)request;
    *((int *)user_data) = *((int *)user_data) + 1;
    int cnt = *((int *)user_data);
    const unsigned int code = 200;
    json_t *counter = json_object();
    json_object_set_new(counter, "code", json_integer(code));
    json_object_set_new(counter, "description", json_integer(cnt));
    ulfius_set_json_body_response(response, code, counter);
    return U_CALLBACK_CONTINUE;
}

int main(void)
{
    struct _u_instance instance;
    int count = 0;

    // Initialize instance with the port number
    if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK)
    {
        fprintf(stderr, "Error ulfius_init_instance, abort\n");
        return (1);
    }

    // Endpoint list declaration
    ulfius_add_endpoint_by_val(&instance, "GET", "/contador/helloworld", NULL, 0, &callback_hello_world, NULL);
    ulfius_add_endpoint_by_val(&instance, "GET", "/contador/value", NULL, 0, &callback_value, &count);
    ulfius_add_endpoint_by_val(&instance, "POST", "/contador/increment", NULL, 0, &callback_increment, &count);

    // Start the framework
    if (ulfius_start_framework(&instance) == U_OK)
    {
        printf("Start framework on port %d\n", instance.port);

        // Wait for the user to press <Enter> on the console to quit the application
        // getchar();
    }
    else
    {
        fprintf(stderr, "Error starting framework\n");
        exit(EXIT_FAILURE);
    }

    /*while (1)
    {
        // gestionar cierre
    }*/
    pause();
    printf("End framework\n");

    ulfius_stop_framework(&instance);
    ulfius_clean_instance(&instance);

    exit(EXIT_SUCCESS);
}