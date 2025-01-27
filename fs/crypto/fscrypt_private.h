/*
 * fscrypt_private.h
 *
 * Copyright (C) 2015, Google, Inc.
 *
 * This contains encryption key functions.
 *
 * Written by Michael Halcrow, Ildar Muslukhov, and Uday Savagaonkar, 2015.
 */

#ifndef _FSCRYPT_PRIVATE_H
#define _FSCRYPT_PRIVATE_H

#define __FS_HAS_ENCRYPTION 1
#include <linux/fscrypt.h>
#include <crypto/hash.h>

#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
#include "fscrypt_knox_private.h"
#endif

#ifdef CONFIG_FSCRYPT_SDP
#include "sdp/fscrypto_sdp_private.h"
#include <sdp/fs_request.h>
#endif

/* Encryption parameters */
#define FS_IV_SIZE			16
#define FS_AES_128_ECB_KEY_SIZE		16
#define FS_AES_128_CBC_KEY_SIZE		16
#define FS_AES_128_CTS_KEY_SIZE		16
#define FS_AES_256_GCM_KEY_SIZE		32
#define FS_AES_256_CBC_KEY_SIZE		32
#define FS_AES_256_CTS_KEY_SIZE		32
#define FS_AES_256_XTS_KEY_SIZE		64

#define FS_KEY_DERIVATION_NONCE_SIZE		16

/**
 * Encryption context for inode
 *
 * Protector format:
 *  1 byte: Protector format (1 = this version)
 *  1 byte: File contents encryption mode
 *  1 byte: File names encryption mode
 *  1 byte: Flags
 *  8 bytes: Master Key descriptor
 *  16 bytes: Encryption Key derivation nonce
 */
struct fscrypt_context {
	u8 format;
	u8 contents_encryption_mode;
	u8 filenames_encryption_mode;
	u8 flags;
	u8 master_key_descriptor[FS_KEY_DESCRIPTOR_SIZE];
	u8 nonce[FS_KEY_DERIVATION_NONCE_SIZE];
#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	u32 knox_flags;
#endif
} __packed;

#define FS_ENCRYPTION_CONTEXT_FORMAT_V1		1

/**
 * For encrypted symlinks, the ciphertext length is stored at the beginning
 * of the string in little-endian format.
 */
struct fscrypt_symlink_data {
	__le16 len;
	char encrypted_path[1];
} __packed;

/*
 * A pointer to this structure is stored in the file system's in-core
 * representation of an inode.
 */
struct fscrypt_info {
	u8 ci_data_mode;
	u8 ci_filename_mode;
	u8 ci_flags;
	struct crypto_skcipher *ci_ctfm;
	struct crypto_cipher *ci_essiv_tfm;
	u8 ci_master_key[FS_KEY_DESCRIPTOR_SIZE];

#ifdef CONFIG_DDAR
	struct dd_info *ci_dd_info;
#endif
#ifdef CONFIG_FSCRYPT_SDP
	struct sdp_info *ci_sdp_info;
#endif
};

typedef enum {
	FS_DECRYPT = 0,
	FS_ENCRYPT,
} fscrypt_direction_t;

#define FS_CTX_REQUIRES_FREE_ENCRYPT_FL		0x00000001
#define FS_CTX_HAS_BOUNCE_BUFFER_FL		0x00000002

static inline bool fscrypt_valid_enc_modes(u32 contents_mode,
					   u32 filenames_mode)
{
	if (contents_mode == FS_ENCRYPTION_MODE_AES_128_CBC &&
	    filenames_mode == FS_ENCRYPTION_MODE_AES_128_CTS)
		return true;

	if (contents_mode == FS_ENCRYPTION_MODE_AES_256_XTS &&
	    filenames_mode == FS_ENCRYPTION_MODE_AES_256_CTS)
		return true;

	if (contents_mode == FS_ENCRYPTION_MODE_SPECK128_256_XTS &&
	    filenames_mode == FS_ENCRYPTION_MODE_SPECK128_256_CTS)
		return true;

	if (contents_mode == FS_PRIVATE_ENCRYPTION_MODE_AES_256_XTS &&
	    filenames_mode == FS_ENCRYPTION_MODE_AES_256_CTS)
		return true;

	if (contents_mode == FS_PRIVATE_ENCRYPTION_MODE_AES_256_CBC &&
	    filenames_mode == FS_ENCRYPTION_MODE_AES_256_CTS)
		return true;

	return false;
}

/* crypto.c */
extern struct kmem_cache *fscrypt_info_cachep;
extern int fscrypt_initialize(unsigned int cop_flags);
extern int fscrypt_do_page_crypto(const struct inode *inode,
				  fscrypt_direction_t rw, u64 lblk_num,
				  struct page *src_page,
				  struct page *dest_page,
				  unsigned int len, unsigned int offs,
				  gfp_t gfp_flags);
extern struct page *fscrypt_alloc_bounce_page(struct fscrypt_ctx *ctx,
					      gfp_t gfp_flags);

/* fname.c */
extern int fname_encrypt(struct inode *inode, const struct qstr *iname,
			 u8 *out, unsigned int olen);
extern bool fscrypt_fname_encrypted_size(const struct inode *inode,
					 u32 orig_len, u32 max_len,
					 u32 *encrypted_len_ret);

/* keyinfo.c */
extern void __exit fscrypt_essiv_cleanup(void);

static inline int __fscrypt_disk_encrypted(const struct inode *inode)
{
#if IS_ENABLED(CONFIG_FS_ENCRYPTION)
#if IS_ENABLED(CONFIG_CRYPTO_DISKCIPHER)
	if (inode && inode->i_crypt_info)
		return S_ISREG(inode->i_mode) && (inode->i_crypt_info->ci_dtfm != NULL);
#endif
#endif
	return 0;
}
#endif /* _FSCRYPT_PRIVATE_H */
