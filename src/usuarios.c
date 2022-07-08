#include <ulfius.h>
#include <jansson.h>
#include <yder.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <regex.h>
#include <time.h>

#define PORT 8082 // El puerto de la API de usuarios
#define MAX_LOG_SIZE 255

// Callback function for the web application on /api/helloworld url call
int callback_hello_world(const struct _u_request *request, struct _u_response *response, void *user_data)
{
    (void)request;
    (void)user_data;
    json_t *hello = json_object();
    json_object_set_new(hello, "text", json_string("Hello World!"));
    ulfius_set_json_body_response(response, 200, hello);
    return U_CALLBACK_CONTINUE;
}

//Retorna el id de usuario
int get_user_id(const char *username)
{
    struct passwd *pw = NULL;
    pw = getpwnam(username);
    if (pw != NULL)
        return (int)pw->pw_uid;
    else
        return -1;
}

//Crea un nuevo usuario en el sistema
int create_user(const char *username, const char *password)
{
    char *command;
    FILE *bash;
    char *command_str = "sudo useradd %s -p $(openssl passwd -5 '%s')";
    
    command = (char *)malloc((strlen(command_str) + strlen(username) + strlen(password)) * sizeof(char));
    sprintf(command, command_str, username, password);

    if ((bash = popen(command, "r")) == NULL) //Es un pipe open al que se le pasa el comando con permisos de lectura
        return -1;

    pclose(bash);
    free(command);
    return 0;
}

void counter_increment(json_t *json_body)
{
    struct _u_request inc_request;
    struct _u_response inc_response;
    ulfius_init_request(&inc_request);
    ulfius_init_response(&inc_response);

    ulfius_set_request_properties(&inc_request,
                                  U_OPT_HTTP_VERB, "POST",
                                  U_OPT_HTTP_URL, "http://www.contadordeusuarios.com",
                                  U_OPT_HTTP_URL_APPEND, "/contador/increment",
                                  U_OPT_AUTH_BASIC, "fred", "fred",
                                  U_OPT_JSON_BODY, json_body,
                                  U_OPT_NONE); // Required to close the parameters list

    int temp = ulfius_send_http_request(&inc_request, &inc_response);
    if (!temp == U_OK)
        perror("Error in HTTP request increment\n");
    ulfius_clean_response(&inc_response);
}

// Callback function for the web application on /api/users url call
int callback_get_users(const struct _u_request *request, struct _u_response *response, void *user_data)
{
    (void)request;
    (void)user_data;

    json_t *array = json_array();
    int users = 0;
    char log[MAX_LOG_SIZE];
    json_t *json_response, *aux;
    // Se lee uno por uno los usuarios de /etc/passwd
    struct passwd *user = getpwent();
    while (user != NULL)
    {
        aux = json_pack("{s:i, s:s}", "user_id", user->pw_uid, "username", user->pw_name);
        json_array_append(array, aux);
        users++;
        user = getpwent();
    }
    // Se anula el acceso a /etc/passwd
    endpwent();
    json_response = json_pack("{s:o}", "data", array);
    ulfius_set_json_body_response(response, 200, json_response);

    sprintf(log, "Cantidad de Usuarios del Sistema: %d", users);
    y_log_message(Y_LOG_LEVEL_INFO, log);

    return U_CALLBACK_CONTINUE;
}

// Callback function for the web application on /api/users url call
int callback_post_users(const struct _u_request *request, struct _u_response *response, void *user_data)
{
    (void)user_data;
    json_t *json_response;
    const char *username, *password;
    char log[MAX_LOG_SIZE];
    //Se recupera la data en json
    json_t *json_data = ulfius_get_json_body_request(request, NULL);
    username = json_string_value(json_object_get(json_data, "username"));
    password = json_string_value(json_object_get(json_data, "password"));

    //Se usa una regex para asegurarse que se ingresaron letras minusculas y no comandos peligrosos
    regex_t lowercase;
    if (regcomp(&lowercase, "[^a-z]", 0) != 0)
        perror("Error con la regex de minusculas");

    // Se checkea que los campos no contengan cosas raras o esten vacios
    if (regcomp(&lowercase, "[^a-z]", 0) != 0 || username == NULL || password == NULL)
    {
        json_response = json_pack("{s:s}", "Error", "Credenciales inválidas, debe ingresar un usuario y contraseña no nulos y solo usando letras minusculas.");
        sprintf(log, "Error: Credenciales inválidas.");
        y_log_message(Y_LOG_LEVEL_ERROR, log);
        ulfius_set_json_body_response(response, 400, json_response); // 400 = Bad Request
        return U_CALLBACK_COMPLETE;
    }

    // Se checkea si el usario ya existe
    if (get_user_id(username) >= 0)
    {
        json_response = json_pack("{s:s}", "Error", "El usuario ya existe.");
        sprintf(log, "Error: Usuario ya existe.");
        y_log_message(Y_LOG_LEVEL_ERROR, log);
        ulfius_set_json_body_response(response, 200, json_response);
        return U_CALLBACK_CONTINUE;
    }

    // Se crea efectivamente el nuevo usuario en el sistema
    if (create_user(username, password) < 0)
    {
        json_response = json_pack("{s:s}", "Error", "Fallo en la creacion del usuario.");
        sprintf(log, "Error: Fallo en la creacion del usuario.");
        y_log_message(Y_LOG_LEVEL_ERROR, log);
        ulfius_set_json_body_response(response, 200, json_response);
        return U_CALLBACK_CONTINUE;
    }

    // Se verifica que se creo el usuario buscandolo en la lista de usuarios
    struct passwd *user = getpwent();
    while (user != NULL)
    {
        if (strcmp(user->pw_name, username) == 0) //Cuando se encuentra el usuario
        {
            time_t now;
            time(&now);
            json_response = json_pack("{s:i, s:s, s:s}", "id", user->pw_uid, "username", user->pw_name, "created_at", ctime(&now));
            //TODO: REVISARRRR FALTA INCREMENTAR CONTADOR 
            counter_increment(json_pack("{s,s}", "ip", u_map_enum_values(request->map_header)[1]));
            ulfius_set_json_body_response(response, 200, json_response); // 200 = Completed
            endpwent();
            sprintf(log, "Usuario creado con ID: %d", user->pw_uid);
            y_log_message(Y_LOG_LEVEL_INFO, log);
            return U_CALLBACK_CONTINUE; //Se retorna de la funcion, lo demás es por errores
        }
        user = getpwent();
    }
    // Si no se encuentra el usuario
    endpwent();
    json_response = json_pack("{s:s}", "Error", "No se encuentra el usuario creado.");
    sprintf(log, "Error: No se encuentra el usuario creado.");
    y_log_message(Y_LOG_LEVEL_ERROR, log);
    ulfius_set_json_body_response(response, 200, json_response);

    return U_CALLBACK_CONTINUE;
}

int main(void)
{
    struct _u_instance instance;

    // Initialize instance with the port number
    if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK)
    {
        fprintf(stderr, "Error ulfius_init_instance, abort\n");
        return (1);
    }

    // Endpoint list declaration
    ulfius_add_endpoint_by_val(&instance, "GET", "/api/helloworld", NULL, 0, &callback_hello_world, NULL);
    ulfius_add_endpoint_by_val(&instance, "GET", "/api/users", NULL, 0, &callback_get_users, NULL);
    ulfius_add_endpoint_by_val(&instance, "POST", "/api/users", NULL, 0, &callback_post_users, NULL);

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

    if (!y_init_logs("usuarios.service", Y_LOG_MODE_FILE, Y_LOG_LEVEL_INFO, "/var/log/usuarios/usuarios.log", "Inicializando usuarios.service"))
    {
        fprintf(stderr, "Error con Yder logs init\n");
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
    y_close_logs();

    exit(EXIT_SUCCESS);
}