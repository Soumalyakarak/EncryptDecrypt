#include "Cryption.hpp"
#include "../processes/Task.hpp"
#include "../FileHandling/ReadEnv.cpp"

#include <openssl/evp.h>  // High-level AES API
#include <openssl/rand.h> // Secure random generator (salt, IV)
#include <openssl/sha.h>  // SHA256 for PBKDF2

#include <vector>
#include <fstream>
#include <iostream>
#include <cstring> // memcpy

#ifdef MULTITHREAD
#include "BenchmarkLogger2.hpp"
#define BENCHMARK BenchmarkLogger2
#else
#include "BenchmarkLogger.hpp"
#define BENCHMARK BenchmarkLogger
#endif

const int SALT_SIZE = 16;
const int IV_SIZE = 12;
const int TAG_SIZE = 16;
const int KEY_SIZE = 32;
const int HEADER_SIZE = SALT_SIZE + IV_SIZE + TAG_SIZE;

bool shouldSkipFile(const std::string &filePath, Action action)
{
    if (action == Action::DECRYPT)
    {
        return filePath.size() < 4 ||
               filePath.substr(filePath.size() - 4) != ".enc";
    }
    else if (action == Action::ENCRYPT)
    {
        return filePath.size() >= 4 &&
               filePath.substr(filePath.size() - 4) == ".enc";
    }
    return false;
}

int encryptFile(const std::string &filePath,
                const std::vector<unsigned char> &fileData,
                const std::string &password)
{
    unsigned char key[KEY_SIZE], salt[SALT_SIZE], iv[IV_SIZE];

    if (RAND_bytes(salt, SALT_SIZE) != 1 || RAND_bytes(iv, IV_SIZE) != 1)
    {
        std::cerr << "Random generation failed\n";
        return -1;
    }

    if (!PKCS5_PBKDF2_HMAC(
            password.c_str(), password.length(),
            salt, SALT_SIZE,
            100000,
            EVP_sha256(),
            KEY_SIZE, key))
    {
        std::cerr << "Key derivation failed\n";
        return -1;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        std::cerr << "Context allocation failed\n";
        return -1;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    std::vector<unsigned char> ciphertext(fileData.size() + TAG_SIZE);
    int len = 0, ciphertext_len = 0;

    if (EVP_EncryptUpdate(ctx,
                          ciphertext.data(), &len,
                          fileData.data(), fileData.size()) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    ciphertext_len += len;

    unsigned char tag[TAG_SIZE];
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_free(ctx);

    std::ofstream out(filePath + ".enc", std::ios::binary);
    if (!out)
    {
        std::cerr << "Output file open failed\n";
        return -1;
    }

    out.write((char *)salt, SALT_SIZE);
    out.write((char *)iv, IV_SIZE);
    out.write((char *)tag, TAG_SIZE);
    out.write((char *)ciphertext.data(), ciphertext_len);
    out.close();

    OPENSSL_cleanse(key, KEY_SIZE);

    BENCHMARK::record_crypto_completion(filePath, true);
    return 0;
}

int decryptFile(const std::string &filePath,
                const std::vector<unsigned char> &fileData,
                const std::string &password)
{
    if (fileData.size() < HEADER_SIZE)
        return 0;

    unsigned char key[KEY_SIZE], salt[SALT_SIZE], iv[IV_SIZE], tag[TAG_SIZE];

    memcpy(salt, fileData.data(), SALT_SIZE);
    memcpy(iv, fileData.data() + SALT_SIZE, IV_SIZE);
    memcpy(tag, fileData.data() + SALT_SIZE + IV_SIZE, TAG_SIZE);

    std::vector<unsigned char> ciphertext(
        fileData.begin() + HEADER_SIZE,
        fileData.end());

    if (!PKCS5_PBKDF2_HMAC(
            password.c_str(), password.length(),
            salt, SALT_SIZE,
            100000,
            EVP_sha256(),
            KEY_SIZE, key))
    {
        std::cerr << "Key derivation failed\n";
        return -1;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        std::cerr << "Context allocation failed\n";
        return -1;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    std::vector<unsigned char> plaintext(ciphertext.size());
    int len = 0, plaintext_len = 0;

    if (EVP_DecryptUpdate(ctx,
                          plaintext.data(), &len,
                          ciphertext.data(), ciphertext.size()) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    plaintext_len = len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, tag) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    std::ofstream out(filePath + ".dec", std::ios::binary);
    if (!out)
    {
        std::cerr << "Output file open failed\n";
        return -1;
    }

    out.write((char *)plaintext.data(), plaintext_len);
    out.close();

    OPENSSL_cleanse(key, KEY_SIZE);

    BENCHMARK::record_crypto_completion(filePath, false);
    return 0;
}

int executeCryption(const std::string &taskData)
{
    try
    {
        Task task = Task::fromString(taskData);
        ReadEnv env;
        std::string password = env.getenv();

        if (shouldSkipFile(task.filePath, task.action))
            return 0;

        std::ifstream in(task.filePath, std::ios::binary);
        if (!in)
        {
            std::cerr << "File open failed\n";
            return -1;
        }

        std::vector<unsigned char> fileData(
            (std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>());
        in.close();

        if (task.action == Action::ENCRYPT)
            return encryptFile(task.filePath, fileData, password);
        else
            return decryptFile(task.filePath, fileData, password);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[CRYPTO ERROR] " << e.what() << std::endl;
        return -1;
    }
}