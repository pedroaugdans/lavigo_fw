/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   base64_coding.h
 * Author: independent contractor
 *
 * Created on November 5, 2019, 1:03 PM
 */

#ifndef BASE64_CODING_H
#define BASE64_CODING_H

char *b64_encode(const unsigned char *in, char * out, size_t len);
int b64_decode(const char *in, unsigned char *out);
size_t b64_encoded_size(size_t inlen);

#endif /* BASE64_CODING_H */

