#include "Cryption.hpp"
#include "../processes/Task.hpp"
#include "../FileHandling/ReadEnv.cpp"

#include <openssl/evp.h>   // High-level AES API
#include <openssl/rand.h>  // Secure random generator (salt, IV)
#include <openssl/sha.h>   // SHA256 for PBKDF2

#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>         // memcpy

#ifdef MULTITHREAD
#include "BenchmarkLogger2.hpp"
#define BENCHMARK BenchmarkLogger2
#else
#include "BenchmarkLogger.hpp"
#define BENCHMARK BenchmarkLogger
#endif

int executeCryption(const std::string &taskData)
{
    try
    {
        // STEP 1: Parse task and fetch password
        Task task = Task::fromString(taskData);
        ReadEnv env;
        std::string password = env.getenv();  // Password used for encryption/decryption

        // STEP 2: Open file in binary mode
        std::ifstream in(task.filePath, std::ios::binary);
        if (!in)
        {
            std::cerr << "File open failed\n";
            return -1;
        }

        // STEP 3: Read entire file into memory buffer
        // (AES works on buffers, not single characters)
        std::vector<unsigned char> fileData(
            (std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>()
        );
        in.close();

        // STEP 4: Allocate space for derived key (256-bit = 32 bytes)
        unsigned char key[32];

        // ============================
        // 🔒 ENCRYPTION FLOW
        // ============================
        if (task.action == Action::ENCRYPT)
        {
            // STEP 5: Generate random salt (for key derivation)
            unsigned char salt[16];
            RAND_bytes(salt, 16);

            // STEP 6: Generate random IV (nonce for GCM)
            unsigned char iv[12];
            RAND_bytes(iv, 12);

            // STEP 7: Derive key from password using PBKDF2
            // Converts password → strong 256-bit key
            PKCS5_PBKDF2_HMAC(
                password.c_str(), password.length(),
                salt, 16,
                100000,              // iteration count (slow = secure)
                EVP_sha256(),
                32, key
            );

            // STEP 8: Initialize AES-256-GCM encryption context
            EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
            EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv);

            // STEP 9: Encrypt data
            std::vector<unsigned char> ciphertext(fileData.size() + 16);
            int len = 0, ciphertext_len = 0;

            EVP_EncryptUpdate(ctx,
                ciphertext.data(), &len,
                fileData.data(), fileData.size());

            ciphertext_len = len;

            // STEP 10: Finalize encryption
            EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
            ciphertext_len += len;

            // STEP 11: Get authentication tag (for integrity check)
            unsigned char tag[16];
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);

            EVP_CIPHER_CTX_free(ctx);

            // STEP 12: Write encrypted output file
            // File structure: [salt][iv][tag][ciphertext]
            std::ofstream out(task.filePath + ".enc", std::ios::binary);

            out.write((char*)salt, 16);               // 16 bytes salt
            out.write((char*)iv, 12);                 // 12 bytes IV
            out.write((char*)tag, 16);                // 16 bytes tag
            out.write((char*)ciphertext.data(), ciphertext_len);

            out.close();

            // Log success
            BENCHMARK::record_crypto_completion(task.filePath, true);
        }

        // ============================
        // 🔓 DECRYPTION FLOW
        // ============================
        else
        {
            // STEP 5: Validate minimum file size
            if (fileData.size() < 44)
            {
                std::cerr << "Invalid encrypted file\n";
                return -1;
            }

            // STEP 6: Extract metadata from file
            unsigned char salt[16];
            unsigned char iv[12];
            unsigned char tag[16];

            memcpy(salt, fileData.data(), 16);
            memcpy(iv, fileData.data() + 16, 12);
            memcpy(tag, fileData.data() + 28, 16);

            // STEP 7: Extract ciphertext
            std::vector<unsigned char> ciphertext(
                fileData.begin() + 44,
                fileData.end()
            );

            // STEP 8: Re-derive key using same password + salt
            PKCS5_PBKDF2_HMAC(
                password.c_str(), password.length(),
                salt, 16,
                100000,
                EVP_sha256(),
                32, key
            );

            // STEP 9: Initialize decryption context
            EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
            EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv);

            // STEP 10: Decrypt data
            std::vector<unsigned char> plaintext(ciphertext.size());
            int len = 0, plaintext_len = 0;

            EVP_DecryptUpdate(ctx,
                plaintext.data(), &len,
                ciphertext.data(), ciphertext.size());

            plaintext_len = len;

            // STEP 11: Set expected authentication tag
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag);

            // STEP 12: Verify integrity + finalize
            if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) <= 0)
            {
                std::cerr << "Decryption failed (wrong password or tampered file)\n";
                EVP_CIPHER_CTX_free(ctx);
                return -1;
            }

            plaintext_len += len;
            EVP_CIPHER_CTX_free(ctx);

            // STEP 13: Write decrypted output file
            std::ofstream out(task.filePath + ".dec", std::ios::binary);
            out.write((char*)plaintext.data(), plaintext_len);
            out.close();

            // Log success
            BENCHMARK::record_crypto_completion(task.filePath, false);
        }

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[CRYPTO ERROR] " << e.what() << std::endl;
        return -1;
    }
}
