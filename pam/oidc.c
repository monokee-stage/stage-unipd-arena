#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * Compile with
 * gcc -o test_iddawc test_iddawc.c -liddawc
 */
#include <stdio.h>
#include <iddawc.h>

#define MAX_USERFILE_SIZE 1024
#define USERSFILE "users"

bool auth_user(const char *, const char *);
void change_pass(const char *, const char *);
/**
 * @brief R
 *
 * @param user
 * @param password
 */
bool auth_user(const char *user, const char *password)
{
    int pos = 0;
    bool authenticated = false;

    if (strcmp(password, "pizza") == 0)
    {
        authenticated = true;
    }

    return authenticated;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *handle, int flags, int argc,
                                   const char **argv)
{
    int pam_code;

    const char *username = NULL;
    const char *password = NULL;

    /* Asking the application for an  username */
    // pam_code = pam_get_user(handle, &username, "username: ");
    // if (pam_code != PAM_SUCCESS)
    // {
    //     fprintf(stderr, "Can't get username");
    //     return PAM_PERM_DENIED;
    // }

    /* Asking the application for a password */
    pam_code =
        pam_get_authtok(handle, PAM_AUTHTOK, &password, "type 'pizza' to login: ");
    if (pam_code != PAM_SUCCESS)
    {
        fprintf(stderr, "Can't get password");
        return PAM_PERM_DENIED;
    }

    /* Checking the PAM_DISALLOW_NULL_AUTHTOK flag: if on, we can't accept empty passwords */
    if (flags & PAM_DISALLOW_NULL_AUTHTOK)
    {
        if (password == NULL || strcmp(password, "") == 0)
        {
            fprintf(stderr,
                    "Null authentication token is not allowed!.");
            return PAM_PERM_DENIED;
        }
    }

    /*Auth user reads a file with usernames and passwords and returns true if username
     * and password are correct. Obviously, you must not save clear text passwords */
    if (auth_user(username, password))
    {
        return PAM_SUCCESS;
    }
    else
    {
        fprintf(stderr, "Wrong username or password");
        return PAM_PERM_DENIED;
    }
}

// PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc,
//                                    const char **argv)
// {
//     int fd;
//     mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
//     char *filename = "/tmp/test.txt";
//     fd = creat(filename, mode);

//     return PAM_SUCCESS;
// }

// PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc,
//                                     const char **argv)
// {
//     remove("/tmp/test.txt");
//     return PAM_SUCCESS;
// }

int main()
{
    struct _i_session i_session;

    i_init_session(&i_session);
    i_set_parameter_list(&i_session, I_OPT_RESPONSE_TYPE, I_RESPONSE_TYPE_ID_TOKEN | I_RESPONSE_TYPE_CODE,
                         I_OPT_OPENID_CONFIG_ENDPOINT, "https://oidc.tld/.well-known/openid-configuration",
                         I_OPT_CLIENT_ID, "testipa",
                         I_OPT_REDIRECT_URI, "https://www.google.com",
                         I_OPT_SCOPE, "openid",
                         I_OPT_STATE_GENERATE, 16,
                         I_OPT_NONCE_GENERATE, 32,
                         I_OPT_NONE);
    if (i_get_openid_config(&i_session))
    {
        fprintf(stderr, "Error loading openid-configuration\n");
        i_clean_session(&i_session);
        return 1;
    }

    // First step: get redirection to login page
    if (i_build_auth_url_get(&i_session))
    {
        fprintf(stderr, "Error building auth request\n");
        i_clean_session(&i_session);
        return 1;
    }
    printf("Redirect to: %s\n", i_get_str_parameter(&i_session, I_OPT_REDIRECT_TO));

    // When the user has logged in the external application, gets redirected with a result, we parse the result
    fprintf(stdout, "Enter redirect URL\n");
    // fgets(redirect_to, 4096, stdin);
    // redirect_to[strlen(redirect_to) - 1] = '\0';
    // i_set_str_parameter(&i_session, I_OPT_REDIRECT_TO, redirect_to);
    if (i_parse_redirect_to(&i_session) != I_OK)
    {
        fprintf(stderr, "Error parsing redirect_to url\n");
        i_clean_session(&i_session);
        return 1;
    }

    // Run the token request, get the refresh and access tokens
    if (i_run_token_request(&i_session) != I_OK)
    {
        fprintf(stderr, "Error running token request\n");
        i_clean_session(&i_session);
        return 1;
    }

    // And finally we load user info using the access token
    if (i_get_userinfo(&i_session, 0) != I_OK)
    {
        fprintf(stderr, "Error loading userinfo\n");
        i_clean_session(&i_session);
        return 1;
    }

    fprintf(stdout, "userinfo:\n%s\n", i_get_str_parameter(&i_session, I_OPT_USERINFO));

    // Cleanup session
    i_clean_session(&i_session);

    return 0;
}