#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

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

void checkAccessTokenValidity(char* access_token);

#endif // AUTHENTICATION_H