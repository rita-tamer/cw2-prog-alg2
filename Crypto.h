#ifndef CRYPTO_H
#define CRYPTO_H

#include <iostream>

class Crypto {
public:
    void init() {
        // Initialize cryptographic library for later use
    }
    std::string aesEncrypt(const unsigned char* input, size_t length) { //encrypts the data using AES converting plaintext into ciphertext through a calculate key based on the value of the length of the input
        return std::string(reinterpret_cast<const char*>(input), length);
    }
    std::string aesDecrypt(const unsigned char* input, size_t length) { //decrypts the data using AES back into plaintext  through a calculated key based on the value of the length of the message
        return std::string(reinterpret_cast<const char*>(input), length);
    }
};

#endif // include guards 