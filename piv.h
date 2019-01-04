/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017, Joyent Inc
 * Author: Alex Wilson <alex.wilson@joyent.com>
 */

#if !defined(_PIV_H)
#define _PIV_H

#include <stdint.h>
#include <assert.h>

#if defined(__APPLE__)
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#else
#include <wintypes.h>
#include <winscard.h>
#endif

#include <sys/types.h>

#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "erf.h"
#include "libssh/digest.h"

extern boolean_t piv_full_apdu_debug;

enum iso_class {
	CLA_ISO = 0x00,
	CLA_CHAIN = 0x10
};

enum iso_sel_p1 {
	SEL_APP_AID = 0x04
};

enum iso_ins {
	/* Standard commands from ISO7816-4 */
	INS_SELECT = 0xA4,
	INS_GET_DATA = 0xCB,
	INS_VERIFY = 0x20,
	INS_CHANGE_PIN = 0x24,
	INS_RESET_PIN = 0x2C,
	INS_GEN_AUTH = 0x87,
	INS_PUT_DATA = 0xDB,
	INS_GEN_ASYM = 0x47,
	INS_CONTINUE = 0xC0,

	/* YubicoPIV specific */
	INS_SET_MGMT = 0xFF,
	INS_IMPORT_ASYM = 0xFE,
	INS_GET_VER = 0xFD,
	INS_SET_PIN_RETRIES = 0xFA,
	INS_ATTEST = 0xF9,
};

enum iso_sw {
	SW_NO_ERROR = 0x9000,
	SW_FUNC_NOT_SUPPORTED = 0x6A81,
	SW_CONDITIONS_NOT_SATISFIED = 0x6985,
	SW_SECURITY_STATUS_NOT_SATISFIED = 0x6982,
	SW_BYTES_REMAINING_00 = 0x6100,
	SW_WARNING_NO_CHANGE_00 = 0x6200,
	SW_WARNING_EOF = 0x6282,
	SW_WARNING_00 = 0x6300,
	SW_FILE_NOT_FOUND = 0x6A82,
	SW_INCORRECT_PIN = 0x63C0,
	SW_INCORRECT_P1P2 = 0x6A86,
	SW_WRONG_DATA = 0x6A80,
	SW_OUT_OF_MEMORY = 0x6A84,
	SW_WRONG_LENGTH = 0x6700,
};

enum piv_sel_tag {
	PIV_TAG_APT = 0x61,
	PIV_TAG_AID = 0x4F,
	PIV_TAG_AUTHORITY = 0x79,
	PIV_TAG_APP_LABEL = 0x50,
	PIV_TAG_URI = 0x5F50,
	PIV_TAG_ALGS = 0xAC,
};

enum piv_tags {
	PIV_TAG_CARDCAP = 0x5FC107,
	PIV_TAG_CHUID = 0x5FC102,
	PIV_TAG_SECOBJ = 0x5FC106,
	PIV_TAG_KEYHIST = 0x5FC10C,
	PIV_TAG_DISCOV = 0x7E,
	PIV_TAG_CERT_9A = 0x5FC105,
	PIV_TAG_CERT_9C = 0x5FC10A,
	PIV_TAG_CERT_9D = 0x5FC10B,
	PIV_TAG_CERT_9E = 0x5FC101,

	PIV_TAG_CERT_82 = 0x5FC10D,	/* First retired slot */
	PIV_TAG_CERT_95 = 0x5FC120,	/* Last retired slot */

	PIV_TAG_CERT_YK_ATTESTATION = 0x5FFF01,
};

enum gen_auth_tag {
	GA_TAG_WITNESS = 0x80,
	GA_TAG_CHALLENGE = 0x81,
	GA_TAG_RESPONSE = 0x82,
	GA_TAG_EXP = 0x85,
};

enum piv_alg {
	PIV_ALG_3DES = 0x03,
	PIV_ALG_RSA1024 = 0x06,
	PIV_ALG_RSA2048 = 0x07,
	PIV_ALG_AES128 = 0x08,
	PIV_ALG_AES192 = 0x0A,
	PIV_ALG_AES256 = 0x0C,
	PIV_ALG_ECCP256 = 0x11,
	PIV_ALG_ECCP384 = 0x14,

	/*
	 * Proprietary hack for Javacards running PivApplet -- they don't
	 * support bare ECDSA so instead we have to give them the full input
	 * data and they hash it on the card.
	 */
	PIV_ALG_ECCP256_SHA1 = 0xf0,
	PIV_ALG_ECCP256_SHA256 = 0xf1,
};

enum piv_cert_comp {
	PIV_COMP_GZIP = 1,
	PIV_COMP_NONE = 0,
};

enum piv_certinfo_flags {
	PIV_CI_X509 = (1 << 2),
	PIV_CI_COMPTYPE = 0x03,
};

enum piv_pin {
	PIV_PIN = 0x80,
	PIV_GLOBAL_PIN = 0x00,
	PIV_PUK = 0x81,
	/* We don't really support these yet. */
	PIV_OCC = 0x96,
	PIV_OCC2 = 0x97,
	PIV_PAIRING = 0x98
};

enum piv_slotid {
	PIV_SLOT_9A = 0x9A,
	PIV_SLOT_9B = 0x9B,
	PIV_SLOT_9C = 0x9C,
	PIV_SLOT_9D = 0x9D,
	PIV_SLOT_9E = 0x9E,

	PIV_SLOT_82 = 0x82,
	PIV_SLOT_95 = 0x95,

	PIV_SLOT_F9 = 0xF9,

	PIV_SLOT_PIV_AUTH = PIV_SLOT_9A,
	PIV_SLOT_ADMIN = PIV_SLOT_9B,
	PIV_SLOT_SIGNATURE = PIV_SLOT_9C,
	PIV_SLOT_KEY_MGMT = PIV_SLOT_9D,
	PIV_SLOT_CARD_AUTH = PIV_SLOT_9E,

	PIV_SLOT_RETIRED_1 = PIV_SLOT_82,
	PIV_SLOT_RETIRED_20 = PIV_SLOT_95,

	PIV_SLOT_YK_ATTESTATION = PIV_SLOT_F9,
};

enum ykpiv_pin_policy {
	YKPIV_PIN_DEFAULT = 0x00,
	YKPIV_PIN_NEVER = 0x01,
	YKPIV_PIN_ONCE = 0x02,
	YKPIV_PIN_ALWAYS = 0x03,
};

enum ykpiv_touch_policy {
	YKPIV_TOUCH_DEFAULT = 0x00,
	YKPIV_TOUCH_NEVER = 0x01,
	YKPIV_TOUCH_ALWAYS = 0x02,
	YKPIV_TOUCH_CACHED = 0x03,
};

struct apdubuf {
	uint8_t *b_data;
	size_t b_offset;
	size_t b_size;
	size_t b_len;
};

struct apdu {
	enum iso_class a_cls;
	enum iso_ins a_ins;
	uint8_t a_p1;
	uint8_t a_p2;
	uint8_t a_le;

	struct apdubuf a_cmd;
	uint16_t a_sw;
	struct apdubuf a_reply;
};

struct piv_slot {
	struct piv_slot *ps_next;
	enum piv_slotid ps_slot;
	enum piv_alg ps_alg;
	X509 *ps_x509;
	const char *ps_subj;
	struct sshkey *ps_pubkey;
};

struct piv_token {
	struct piv_token *pt_next;
	const char *pt_rdrname;
	SCARDHANDLE pt_cardhdl;
	DWORD pt_proto;
	SCARD_IO_REQUEST pt_sendpci;
	boolean_t pt_intxn;
	boolean_t pt_reset;

	uint8_t pt_fascn[26];
	size_t pt_fascn_len;

	uint8_t pt_guid[16];
	uint8_t pt_chuuid[16];
	uint8_t pt_expiry[8];
	enum piv_alg pt_algs[32];
	size_t pt_alg_count;
	uint pt_pinretries;
	boolean_t pt_ykpiv;
	boolean_t pt_nochuid;
	boolean_t pt_signedchuid;
	uint8_t pt_ykver[3];

	uint8_t pt_hist_oncard;
	uint8_t pt_hist_offcard;
	char *pt_hist_url;

	enum piv_pin pt_auth;

	boolean_t pt_pin_global;
	boolean_t pt_pin_app;
	boolean_t pt_occ;
	boolean_t pt_vci;

	struct piv_slot *pt_slots;
	struct piv_slot *pt_last_slot;
};

struct piv_ecdh_box {
	boolean_t pdb_guidslot_valid;
	uint8_t pdb_guid[16];
	enum piv_slotid pdb_slot;

	struct sshkey *pdb_ephem_pub;
	struct sshkey *pdb_pub;

	boolean_t pdb_free_str;
	const char *pdb_cipher;
	const char *pdb_kdf;

	struct apdubuf pdb_iv;
	struct apdubuf pdb_enc;
	struct apdubuf pdb_plain;
};

/*
 * Enumerates all PIV tokens attached to the given SCARDCONTEXT.
 *
 * Errors:
 *  - PCSCError: a PCSC call failed in a way that is not retryable
 */
erf_t *piv_enumerate(SCARDCONTEXT ctx, struct piv_token **tokens);

/* Releases a list of tokens acquired from piv_enumerate. */
void piv_release(struct piv_token *pk);

/*
 * Gets a reference to a particular key/cert slot on the card. This must have
 * been enumerated using piv_read_cert, or else this will return NULL.
 */
struct piv_slot *piv_get_slot(struct piv_token *tk, enum piv_slotid slotid);

/* Low-level APDU access */
struct apdu *piv_apdu_make(enum iso_class cls, enum iso_ins ins, uint8_t p1,
    uint8_t p2);
void piv_apdu_free(struct apdu *pdu);

erf_t *piv_apdu_transceive(struct piv_token *pk, struct apdu *pdu);
erf_t *piv_apdu_transceive_chain(struct piv_token *pk, struct apdu *apdu);

/*
 * Begins a new transaction on the card. Needs to be called before any
 * interaction with the card is possible.
 *
 * Errors:
 *  - IOError: general communication failure
 */
erf_t *piv_txn_begin(struct piv_token *key);

/*
 * Ends a transaction.
 */
void piv_txn_end(struct piv_token *key);

/*
 * Selects the PIV applet on the card. You should run this first in each
 * txn to prepare the card for other PIV commands.
 *
 * Errors:
 *  - IOError: general card communication failure
 *  - InvalidDataError: device returned invalid or unsupported payload to
 *                      select command
 *  - NotFoundError: PIV applet not found on card
 */
erf_t *piv_select(struct piv_token *tk);

/*
 * Reads the certificate in a given slot on the card, and updates the list
 * of struct piv_slots with info about it.
 *
 * This is required before commands that require a slot reference can be used
 * (e.g. piv_sign, piv_ecdh).
 *
 * Errors:
 *  - IOError: general card communication failure
 *  - NotFoundError: no key/cert is present in this slot
 *  - NotSupportedError: card does not support the use of this slot
 *  - PermissionError: the cert in this slot requires either using a contact
 *                     interface (and the card is connected contactless), or
 *                     requires a PIN
 *  - InvalidDataError: device returned an invalid payload or unparseable
 *                      certificate
 *  - APDUError: card rejected the request (e.g because applet not selected)
 */
erf_t *piv_read_cert(struct piv_token *tk, enum piv_slotid slotid);
/*
 * Attempts to read certificates in all supported PIV slots on the card, by
 * calling piv_read_cert repeatedly. Ignores ENOENT and ENOTSUP errors. Any
 * other error will return early and may not try all slots.
 */
erf_t *piv_read_all_certs(struct piv_token *tk);

/*
 * Authenticates as the card administrator using a 3DES key.
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - ENOENT: the card has no 3DES admin key
 *  - EACCES: the key was invalid
 *  - EINVAL: the card rejected the command
 */
erf_t *piv_auth_admin(struct piv_token *tk, const uint8_t *key, size_t keylen);

/*
 * YubicoPIV-specific: changes the 3DES card administrator key.
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - ENOENT: the card has no 3DES admin key
 *  - EPERM: must call piv_auth_admin() first
 *  - EINVAL: the card rejected the command or is not YubicoPIV
 */
int ykpiv_set_admin(struct piv_token *tk, const uint8_t *key, size_t keylen,
    enum ykpiv_touch_policy touchpolicy);

/*
 * Generates a new asymmetric private key in a slot on the token, and returns
 * the public key.
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - EPERM: the card requires admin authentication before generating keys
 *  - EINVAL: the card rejected the command
 */
int piv_generate(struct piv_token *tk, enum piv_slotid slotid,
    enum piv_alg alg, struct sshkey **pubkey);

/*
 * YubicoPIV specific: generates a new asymmetric private key in a slot on the
 * token, and returns the public key in the same manner as piv_generate(), but
 * takes two extra arguments for the PIN and Touch policy that can be set with
 * YubicoPIV.
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - EPERM: the card requires admin authentication before generating keys
 *  - EINVAL: the card rejected the command, or you tried to use this on a
 *            non-YubicoPIV card
 */
int ykpiv_generate(struct piv_token *tk, enum piv_slotid slotid,
    enum piv_alg alg, enum ykpiv_pin_policy pinpolicy,
    enum ykpiv_touch_policy touchpolicy, struct sshkey **pubkey);

/*
 * Loads a certificate for a given slot on the token.
 *
 * "flags" should include bits from enum piv_certinfo_flags (and piv_cert_comp).
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - ENOMEM: certificate is too large to fit on card
 *  - EPERM: admin authentication required to write a cert
 *  - ENOENT: slot unsupported
 *  - EINVAL: other card error
 */
int piv_write_cert(struct piv_token *tk, enum piv_slotid slotid,
    const uint8_t *data, size_t datalen, uint flags);

/*
 * Tries to unlock the PIV token using a PIN code.
 *
 * The "pin" argument should be a NULL-terminated ASCII numeric string of the
 * PIN to use. Max length is 8 digits.
 *
 * The boolean "canskip" argument is used to indicate whether PIN entry should
 * be skipped if the PIN has already been entered (we use an empty VERIFY
 * command to check the security status). This should be to B_FALSE before
 * using "PIN Always" slots like the 9C Digital Signature slot.
 *
 * If the argument "retries" is given, then it will be read to determine a
 * minimum number of remaining attempts to assert are possible before trying to
 * unlock: if less than "*retries" attempts are remaining, we will not attempt
 * to unlock and will return EAGAIN.
 *
 * If EACCES is returned, then "retries" will also be written with the new
 * remaining attempts count.
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - EINVAL: the card rejected the command (e.g. because applet not selected)
 *  - EAGAIN: the PIN has a remaining retries count that is too low
 *  - EACCES: the PIN code was incorrect. If non-NULL, the "retries" argument
 *            will be written with the number of attempts remaining before the
 *            card locks itself (and potentially erases keys)
 */
int piv_verify_pin(struct piv_token *tk, enum piv_pin type, const char *pin,
    uint *retries, boolean_t canskip);

/*
 * Changes the PIV PIN on a token.
 *
 * The "pin" and "newpin" arguments should be a NULL-terminated ASCII numeric
 * string of the PIN to use. Max length is 10 digits.
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - EINVAL: the card rejected the command (e.g. because applet not selected)
 *  - EACCES: the old PIN code was incorrect.
 */
int piv_change_pin(struct piv_token *tk, enum piv_pin type, const char *pin,
    const char *newpin);

/*
 * Resets the PIV PIN on a token using the PUK.
 *
 * The "puk" and "newpin" arguments should be a NULL-terminated ASCII numeric
 * string of the PIN to use. Max length is 10 digits.
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - EINVAL: the card rejected the command (e.g. because applet not selected)
 *  - EACCES: the PUK was incorrect.
 */
int piv_reset_pin(struct piv_token *tk, enum piv_pin type, const char *puk,
    const char *newpin);

/*
 * YubicoPIV only: changes the maximum number of retries for the PIN and PUK.
 * This also resets both PIN and PUK to their default values. To execute it
 * you must have called both piv_auth_admin() and piv_verify_pin() in this
 * transaction.
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - EINVAL: the card rejected the command (e.g. because applet not selected)
 *            or the card does not support YubicoPIV extensions
 *  - EPERM: the necessary auth has not been done before calling
 *            (piv_auth_admin() and piv_verify_pin()).
 */
int ykpiv_set_pin_retries(struct piv_token *tk, uint pintries, uint puktries);

/*
 * Authenticates a PIV key slot by matching its public key against the given
 * public key, and then asking it to sign randomly generated data to validate
 * that the key does match.
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - EINVAL: the card rejected the command (e.g. because applet not selected)
 *  - EPERM: the key slot in question is locked
 *  - ENOTSUP: the card returned a GEN_AUTH payload type that isn't supported
 *  - ESRCH: the key validation failed (either because it doesn't match the
 *           provided pubkey, or because the signature did not validate)
 */
int piv_auth_key(struct piv_token *tk, struct piv_slot *slot,
    struct sshkey *pubkey);

/*
 * Requests an attestation certificate.
 *
 * Errors:
 *  - EIO: general card communication failure
 *  - EINVAL: the card rejected the command (e.g. because applet not selected,
 *            or the command is unsupported)
 */
int ykpiv_attest(struct piv_token *tk, struct piv_slot *slot,
    uint8_t **data, size_t *len);

/*
 * Signs a payload using a private key stored on the card.
 *
 * "data" must contain "datalen" bytes of payload that will be signed.
 *
 * "hashalgo" will be written with the SSH digest ID of the hash algorithm that
 * was used. It can also be filled out with a desired hash algorithm before
 * calling (this will probably only work with RSA). We might not be able to give
 * you the algo you asked for (you will need to check it on return).
 *
 * "signature" will be written with a pointer to a buffer "siglen" bytes long
 * containing the output signature in ASN.1/X509 format. It should be released
 * with free().
 *
 * Errors:
 *   - EIO: general card communication failure
 *   - EPERM: the key slot in question is locked and cannot be used. You might
 *            need to unlock the card with piv_verify_pin.
 *   - EINVAL: the card rejected the command (e.g. because applet not selected)
 *   - ENOTSUP: the card returned a GEN_AUTH payload type that isn't supported
 */
int piv_sign(struct piv_token *tk, struct piv_slot *slot, const uint8_t *data,
    size_t datalen, enum sshdigest_types *hashalgo, uint8_t **signature,
    size_t *siglen);
int piv_sign_prehash(struct piv_token *tk, struct piv_slot *slot,
    const uint8_t *hash, size_t hashlen, uint8_t **signature, size_t *siglen);

/*
 * Performs an ECDH key derivation between the private key on the token and
 * the given EC public key.
 *
 * "pubkey" must point at an EC public key.
 *
 * "secret" will be written with a pointer to a buffer "seclen" bytes long
 * containing the output shared secret. It should be released with freezero().
 *
 * Errors:
 *   - EIO: general card communication failure
 *   - EPERM: the key slot in question is locked and cannot be used. You might
 *            need to unlock the card with piv_verify_pin.
 *   - EINVAL: the card rejected the command (e.g. because applet not selected)
 *   - ENOTSUP: the card returned a GEN_AUTH payload type that isn't supported
 */
int piv_ecdh(struct piv_token *tk, struct piv_slot *slot,
    struct sshkey *pubkey, uint8_t **secret, size_t *seclen);

struct piv_ecdh_box *piv_box_new(void);
struct piv_ecdh_box *piv_box_clone(const struct piv_ecdh_box *box);
int piv_box_set_data(struct piv_ecdh_box *box, const uint8_t *data, size_t len);
int piv_box_set_datab(struct piv_ecdh_box *box, struct sshbuf *buf);
int piv_box_seal(struct piv_token *tk, struct piv_slot *slot,
    struct piv_ecdh_box *box);
int piv_box_seal_offline(struct sshkey *pubk, struct piv_ecdh_box *box);
int piv_box_to_binary(struct piv_ecdh_box *box, uint8_t **output, size_t *len);

int piv_box_from_binary(const uint8_t *input, size_t len,
    struct piv_ecdh_box **box);
int piv_box_find_token(struct piv_token *tks, struct piv_ecdh_box *box,
    struct piv_token **tk, struct piv_slot **slot);
int piv_box_open(struct piv_token *tk, struct piv_slot *slot,
    struct piv_ecdh_box *box);
int piv_box_open_offline(struct sshkey *privkey, struct piv_ecdh_box *box);
int piv_box_take_data(struct piv_ecdh_box *box, uint8_t **data, size_t *len);
int piv_box_take_datab(struct piv_ecdh_box *box, struct sshbuf **buf);
void piv_box_free(struct piv_ecdh_box *box);

int sshbuf_put_piv_box(struct sshbuf *buf, struct piv_ecdh_box *box);
int sshbuf_get_piv_box(struct sshbuf *buf, struct piv_ecdh_box **box);

int piv_write_file(struct piv_token *pt, uint tag,
    const uint8_t *data, size_t len);
int piv_read_file(struct piv_token *pt, uint tag, uint8_t **data, size_t *len);

#endif
