/*
 * PASN info for initiator and responder
 *
 * Copyright (C) 2019, Intel Corporation
 * Copyright (c) 2022, Jouni Malinen <j@w1.fi>
 * Copyright (C) 2022, Qualcomm Innovation Center, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef PASN_COMMON_H
#define PASN_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_PASN

enum pasn_fils_state {
	PASN_FILS_STATE_NONE = 0,
	PASN_FILS_STATE_PENDING_AS,
	PASN_FILS_STATE_COMPLETE
};

struct pasn_fils {
	u8 state;
	u8 nonce[FILS_NONCE_LEN];
	u8 anonce[FILS_NONCE_LEN];
	u8 session[FILS_SESSION_LEN];
	u8 erp_pmkid[PMKID_LEN];
	bool completed;
	struct wpabuf *erp_resp;
};

struct pasn_data {
	int akmp;
	int cipher;
	u16 group;
	bool secure_ltf;
	int freq;
	size_t kdk_len;

	u8 trans_seq;
	u8 status;

	u8 own_addr[ETH_ALEN];
	u8 peer_addr[ETH_ALEN];
	u8 bssid[ETH_ALEN];
	size_t pmk_len;
	u8 pmk[PMK_LEN_MAX];
	bool using_pmksa;

	u8 hash[SHA384_MAC_LEN];

	struct wpabuf *beacon_rsne_rsnxe;
	struct wpa_ptk ptk;
	struct crypto_ecdh *ecdh;

	struct wpabuf *comeback;
	u16 comeback_after;

#ifdef CONFIG_SAE
	struct sae_data sae;
	struct sae_pt *pt;
#endif /* CONFIG_SAE */

#ifdef CONFIG_FILS
	bool fils_eapol;
	bool fils_wd_valid;
	struct pasn_fils fils;
#endif /* CONFIG_FILS */

#ifdef CONFIG_IEEE80211R
	u8 pmk_r1[PMK_LEN_MAX];
	size_t pmk_r1_len;
	u8 pmk_r1_name[WPA_PMK_NAME_LEN];
#endif /* CONFIG_IEEE80211R */
	/* Note that this pointers to RSN PMKSA cache are actually defined
	 * differently for the PASN initiator (using RSN Supplicant
	 * implementation) and PASN responser (using RSN Authenticator
	 * implementation). Functions cannot be mixed between those cases. */
	struct rsn_pmksa_cache *pmksa;
	struct rsn_pmksa_cache_entry *pmksa_entry;
	struct eapol_sm *eapol;
	int fast_reauth;
#ifdef CONFIG_TESTING_OPTIONS
	int corrupt_mic;
#endif /* CONFIG_TESTING_OPTIONS */
	void *cb_ctx;
	u16 rsnxe_capab;
	int network_id;

	u8 wrapped_data_format;
	struct wpabuf *secret;

	/* Responder */
	bool noauth; /* Whether PASN without mutual authentication is enabled */
	int wpa_key_mgmt;
	int rsn_pairwise;
	bool derive_kdk;
	const char *password;
	int disable_pmksa_caching;
	int *pasn_groups;
	struct wpabuf *wrapped_data;
	int use_anti_clogging;
	const u8 *rsn_ie;
	const u8 *rsnxe_ie;
	size_t rsn_ie_len;

	u8 *comeback_key;
	struct os_reltime last_comeback_key_update;
	u16 comeback_idx;
	u16 *comeback_pending_idx;

	bool custom_pmkid_valid;
	u8 custom_pmkid[PMKID_LEN];

	/**
	 * Extra elements to add into Authentication frames. These can be used,
	 * e.g., for Wi-Fi Aware use cases.
	 */
	const u8 *extra_ies;
	size_t extra_ies_len;

	/**
	 * send_mgmt - Function handler to transmit a Management frame
	 * @ctx: Callback context from cb_ctx
	 * @frame_buf : Frame to transmit
	 * @frame_len: Length of frame to transmit
	 * @freq: Frequency in MHz for the channel on which to transmit
	 * @wait_dur: How many milliseconds to wait for a response frame
	 * Returns: 0 on success, -1 on failure
	 */
	int (*send_mgmt)(void *ctx, const u8 *data, size_t data_len, int noack,
			 unsigned int freq, unsigned int wait);
	/**
	 * validate_custom_pmkid - Handler to validate vendor specific PMKID
	 * @ctx: Callback context from cb_ctx
	 * @addr : MAC address of the peer
	 * @pmkid: Custom PMKID
	 * Returns: 0 on success (valid PMKID), -1 on failure
	 */
	int (*validate_custom_pmkid)(void *ctx, const u8 *addr,
				     const u8 *pmkid);
};

/* Initiator */

void wpa_pasn_reset(struct pasn_data *pasn);
int wpas_pasn_start(struct pasn_data *pasn, const u8 *own_addr,
		    const u8 *peer_addr, const u8 *bssid,
		    int akmp, int cipher, u16 group,
		    int freq, const u8 *beacon_rsne, u8 beacon_rsne_len,
		    const u8 *beacon_rsnxe, u8 beacon_rsnxe_len,
		    const struct wpabuf *comeback);
int wpa_pasn_verify(struct pasn_data *pasn, const u8 *own_addr,
		    const u8 *peer_addr, const u8 *bssid,
		    int akmp, int cipher, u16 group,
		    int freq, const u8 *beacon_rsne, u8 beacon_rsne_len,
		    const u8 *beacon_rsnxe, u8 beacon_rsnxe_len,
		    const struct wpabuf *comeback);
int wpa_pasn_auth_rx(struct pasn_data *pasn, const u8 *data, size_t len,
		     struct wpa_pasn_params_data *pasn_params);
int wpa_pasn_auth_tx_status(struct pasn_data *pasn,
			    const u8 *data, size_t data_len, u8 acked);

/* Responder */
int handle_auth_pasn_1(struct pasn_data *pasn,
		       const u8 *own_addr, const u8 *peer_addr,
		       const struct ieee80211_mgmt *mgmt, size_t len,
		       bool reject);
int handle_auth_pasn_3(struct pasn_data *pasn, const u8 *own_addr,
		       const u8 *peer_addr,
		       const struct ieee80211_mgmt *mgmt, size_t len);
int handle_auth_pasn_resp(struct pasn_data *pasn, const u8 *own_addr,
			  const u8 *peer_addr,
			  struct rsn_pmksa_cache_entry *pmksa, u16 status);

#endif /* CONFIG_PASN */

#ifdef __cplusplus
}
#endif
#endif /* PASN_COMMON_H */
