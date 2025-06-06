#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <stdint.h>

/**
 * @brief Create a signed JWT
 * @param client_email The client email
 * @param private_key The private key
 * @return The signed JWT
 */
char *create_jwt(const char *client_email, const char *private_key);

/**
 * @brief Exchange a signed JWT for an OAuth 2.0 Bearer token
 * @param jwt The signed JWT
 * @return The OAuth 2.0 Bearer token
 */
char *exchange_jwt_for_access_token(const char *jwt);

/**
 * @brief Generate a new access token using the service account 
 * credentials from keys.h
 */
char *generate_access_token();

/**
 * @brief Check if the access token is still valid, if expired generate a new one and stores it in the provided pointer
 * @param access_token Pointer to the pointer to access token
 */
uint8_t checkAccessTokenValidity(char** access_token);

#endif // AUTHENTICATION_H