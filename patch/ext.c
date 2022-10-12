
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <usbmuxd.h>

#ifdef HAVE_OPENSSL
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#else
#include <gnutls/gnutls.h>
#endif

#include "libimobiledevice/ext.h"
#include "idevice.h"
#include "mobile_image_mounter.h"
#include "common/userpref.h"
#include "common/socket.h"
#include "common/thread.h"
#include "common/debug.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifndef WIN32
#include <netdb.h>
#endif

LIBIMOBILEDEVICE_API idevice_error_t idevice_new_force_network(idevice_t * device, const char *udid, const char* host)
{
	idevice_t _device = (idevice_t)malloc(sizeof(struct idevice_private));
	if (!_device)
		return IDEVICE_E_NO_DEVICE;
	struct hostent* ent;
	struct sockaddr_in sa;
	sa.sin_family = 0x0200; // AF_INET in big endian
	sa.sin_port = 0;
	if (!(ent = gethostbyname(host))){
		return IDEVICE_E_NO_DEVICE;
	}
	sa.sin_addr = *(struct in_addr *) ent->h_addr;
	_device->udid = strdup(udid);
	_device->mux_id = 19900724;
	_device->version = 0;
	_device->conn_type = CONNECTION_NETWORK;
	size_t len = 32;
	_device->conn_data = malloc(len);
	memcpy(_device->conn_data, &sa, sizeof(sa));
	*device = _device;
	return IDEVICE_E_SUCCESS;

}

static ssize_t mim_upload_cb(void* buf, size_t size, void* userdata)
{
	return fread(buf, 1, size, (FILE*)userdata);
}

LIBIMOBILEDEVICE_API mobile_image_mounter_error_t mobile_image_mounter_upload_image_file(mobile_image_mounter_client_t client, const char *image_type, const char* image_file_path, const char* image_signature_file_path)
{
	struct stat fst;
	if (stat(image_file_path, &fst) != 0) {
		return MOBILE_IMAGE_MOUNTER_E_UNKNOWN_ERROR;
	}
	size_t image_size = fst.st_size;

	char sig[8192];
	size_t sig_length = 0;
	FILE *f = fopen(image_signature_file_path, "rb");
	if (!f) {
		return MOBILE_IMAGE_MOUNTER_E_UNKNOWN_ERROR;
	}
	sig_length = fread(sig, 1, sizeof(sig), f);
	fclose(f);
	if (sig_length == 0) {
		return MOBILE_IMAGE_MOUNTER_E_UNKNOWN_ERROR;
	}

	f = fopen(image_file_path, "rb");
	if (!f) {
		return MOBILE_IMAGE_MOUNTER_E_UNKNOWN_ERROR;
	}

	mobile_image_mounter_error_t res = mobile_image_mounter_upload_image(client, image_type, image_size, sig, sig_length, mim_upload_cb, f);
	fclose(f);

	return res;
}

LIBIMOBILEDEVICE_API mobile_image_mounter_error_t mobile_image_mounter_mount_image_file(mobile_image_mounter_client_t client, const char *image_path, const char *signature_file, const char *image_type, plist_t *result)
{
	char sig[8192];
	size_t sig_length = 0;
	FILE *f = fopen(signature_file, "rb");
	if (!f) {
		return MOBILE_IMAGE_MOUNTER_E_UNKNOWN_ERROR;
	}
	sig_length = fread(sig, 1, sizeof(sig), f);
	fclose(f);
	if (sig_length == 0) {
		return MOBILE_IMAGE_MOUNTER_E_UNKNOWN_ERROR;
	}

	return mobile_image_mounter_mount_image(client, image_path, sig, sig_length, image_type, result);
}

LIBIMOBILEDEVICE_API void libimobiledevice_free(void* ptr){
    free(ptr);
} 

struct instrument_client_private {
	service_client_t parent;
};

/**
 * Convert a service_error_t value to a instrument_error_t
 * value. Used internally to get correct error codes.
 *
 * @param err A service_error_t error code
 *
 * @return A matching instrument_error_t error code,
 *     INSTRUMENT_E_UNKNOWN_ERROR otherwise.
 */
static instrument_error_t instrument_error(service_error_t err)
{
	switch (err) {
		case SERVICE_E_SUCCESS:
			return INSTRUMENT_E_SUCCESS;
	    case SERVICE_E_INVALID_ARG:
            return INSTRUMENT_E_INVALID_ARG;
        case SERVICE_E_MUX_ERROR:
            return INSTRUMENT_E_MUX_ERROR;
        case SERVICE_E_SSL_ERROR:
            return INSTRUMENT_E_SSL_ERROR;
		default:
            // printf("unknown service error: %d\n", err);
			break;
	}
	return INSTRUMENT_E_UNKNOWN_ERROR;
}

LIBIMOBILEDEVICE_API instrument_error_t instrument_client_new(idevice_t device, lockdownd_service_descriptor_t service, instrument_client_t * client){
    service_client_t service_client = NULL;
    if (!device || !service || service->port == 0 || !client || *client)
		return INSTRUMENT_E_INVALID_ARG;
    instrument_error_t err = instrument_error(service_client_new(device, service, &service_client));
    if(err != INSTRUMENT_E_SUCCESS){
        return err;
    }

    if (service->identifier && (strcmp(service->identifier, INSTRUMENT_REMOTESERVER_SECURE_SERVICE_NAME) != 0)) {
      service_disable_bypass_ssl(service_client, 1);
    }

    instrument_client_t client_loc = (instrument_client_t) malloc(sizeof(struct instrument_client_private));
    client_loc->parent = service_client;
    *client = client_loc;

    return INSTRUMENT_E_SUCCESS;

}

LIBIMOBILEDEVICE_API instrument_error_t instrument_client_start_service(idevice_t device, instrument_client_t* client, const char* label){
    // internal_set_debug_level(1);
    instrument_error_t err = INSTRUMENT_E_UNKNOWN_ERROR;
    instrument_error_t start_service_error = INSTRUMENT_E_UNKNOWN_ERROR;
	
	// try secure mode
	start_service_error = instrument_error(service_client_factory_start_service(device, INSTRUMENT_REMOTESERVER_SECURE_SERVICE_NAME, (void**)client, label, SERVICE_CONSTRUCTOR(instrument_client_new), &err));
    if (start_service_error == INSTRUMENT_E_SUCCESS){
        return err;
    }
    // fallback
    start_service_error = instrument_error(service_client_factory_start_service(device, INSTRUMENT_REMOTESERVER_SERVICE_NAME, (void**)client, label, SERVICE_CONSTRUCTOR(instrument_client_new), &err));
    if (start_service_error == INSTRUMENT_E_SUCCESS){
        return err;
    }
    return start_service_error;
}

LIBIMOBILEDEVICE_API instrument_error_t instrument_client_free(instrument_client_t client){
    if (!client){
        return INSTRUMENT_E_INVALID_ARG;
    }
    if (client->parent){
        service_client_free(client->parent);
    }
    client->parent = NULL;
    free(client);
    return INSTRUMENT_E_SUCCESS;
}

LIBIMOBILEDEVICE_API instrument_error_t instrument_send_command(instrument_client_t client, const char *data, uint32_t size, uint32_t *sent){
    if (!client || !data || !client->parent){
        return INSTRUMENT_E_INVALID_ARG;
    }
    instrument_error_t err = instrument_error(service_send(client->parent, data, size, sent));
    return err;
}

LIBIMOBILEDEVICE_API instrument_error_t instrument_receive_with_timeout(instrument_client_t client, char *data, uint32_t size, uint32_t *received, unsigned int timeout){
    if (!client || !data || !client->parent){
        return INSTRUMENT_E_INVALID_ARG;
    }
    instrument_error_t err = instrument_error(service_receive_with_timeout(client->parent, data, size, received, timeout));
    return err;
}

LIBIMOBILEDEVICE_API instrument_error_t instrument_receive(instrument_client_t client, char *data, uint32_t size, uint32_t *received){
    if (!client || !data || !client->parent){
        return INSTRUMENT_E_INVALID_ARG;
    }
    instrument_error_t err = instrument_error(service_receive(client->parent, data, size, received));
    return err;
}

/*
	AMFI Service
*/
struct amfi_client_private {
	service_client_t parent;
	mutex_t mutex;
};

/**
 * Locks a amfi client, used for thread safety.
 *
 * @param client amfi client to lock
 */
static void amfi_lock(amfi_client_t client)
{
	mutex_lock(&client->mutex);
}

/**
 * Unlocks a amfi client, used for thread safety.
 *
 * @param client amfi client to unlock
 */
static void amfi_unlock(amfi_client_t client)
{
	mutex_unlock(&client->mutex);
}

static amfi_error_t amfi_process_result(plist_t result)
{
	amfi_error_t res = AMFI_E_COMMAND_FAILED;
	char* strval = NULL;
	uint8_t strval_b;
	plist_t node;

	node = plist_dict_get_item(result, "Error");
	if (node && plist_get_node_type(node) == PLIST_STRING) {
		plist_get_string_val(node, &strval);
	}
	if (strval) {
		if (!strcmp(strval, "Device has a passcode set")) {
			res = AMFI_E_DEVICE_HAS_A_PASSCODE_SET;
		} else {
			res = AMFI_E_UNKNOWN_ERROR;
		}
		free(strval);
		return res;
	}

	node = plist_dict_get_item(result, "success");
	if (node && plist_get_node_type(node) == PLIST_BOOLEAN) {
		plist_get_bool_val(node, &strval_b);
	}
	if (!strval_b) {
		res = AMFI_E_UNKNOWN_ERROR;
	} else {
		res = AMFI_E_SUCCESS;
	}
	free(strval);

	return res;
}

/**
 * Convert a property_list_service_error_t value to a amfi_error_t
 * value. Used internally to get correct error codes.
 *
 * @param err A property_list_service_error_t error code
 *
 * @return A matching amfi_error_t error code,
 *     AMFI_E_UNKNOWN_ERROR otherwise.
 */
static amfi_error_t amfi_error(property_list_service_error_t err)
{
	switch (err) {
		case PROPERTY_LIST_SERVICE_E_SUCCESS:
			return AMFI_E_SUCCESS;
	    case PROPERTY_LIST_SERVICE_E_INVALID_ARG:
            return AMFI_E_INVALID_ARG;
        case PROPERTY_LIST_SERVICE_E_PLIST_ERROR:
            return AMFI_E_PLIST_ERROR;
        case PROPERTY_LIST_SERVICE_E_MUX_ERROR:
            return AMFI_E_CONN_FAILED;
		default:
			break;
	}
	return AMFI_E_UNKNOWN_ERROR;
}

LIBIMOBILEDEVICE_API amfi_error_t amfi_client_new(idevice_t device, lockdownd_service_descriptor_t service, amfi_client_t * client) {
	property_list_service_client_t plistclient = NULL;
	amfi_error_t err = amfi_error(property_list_service_client_new(device, service, &plistclient));
	if (err != AMFI_E_SUCCESS) {
		return err;
	}

	amfi_client_t client_loc = (amfi_client_t) malloc(sizeof(struct amfi_client_private));
	client_loc->parent = plistclient;

	mutex_init(&client_loc->mutex);

	*client = client_loc;
	return AMFI_E_SUCCESS;
}

LIBIMOBILEDEVICE_API amfi_error_t amfi_client_start_service(idevice_t device, amfi_client_t* client, const char* label) {
    amfi_error_t err = AMFI_E_UNKNOWN_ERROR;
    service_client_factory_start_service(device, AMFI_REMOTESERVER_SERVICE_NAME, (void**)client, label, SERVICE_CONSTRUCTOR(amfi_client_new), &err);
	return err;
}

LIBIMOBILEDEVICE_API amfi_error_t amfi_client_free(amfi_client_t client) {
	if (!client) {
		return AMFI_E_INVALID_ARG;
	}

	property_list_service_client_free(client->parent);
	client->parent = NULL;
	mutex_destroy(&client->mutex);
	free(client);

	return AMFI_E_SUCCESS;
}

LIBIMOBILEDEVICE_API amfi_error_t amfi_set_developer_mode(amfi_client_t client, int mode) {
	amfi_lock(client);

	plist_t result = NULL;
	plist_t dict = plist_new_dict();
 	plist_dict_set_item(dict, "action", plist_new_uint(mode));

  	amfi_error_t ret = amfi_error(property_list_service_send_xml_plist(client->parent, dict));
    plist_free(dict);
  	if (ret != AMFI_E_SUCCESS) {
    	goto leave_unlock;
  	}

  	ret = amfi_error(property_list_service_receive_plist(client->parent, &result));
	if (ret != MOBILE_IMAGE_MOUNTER_E_SUCCESS) {
		debug_info("Error receiving response from device!");
		goto leave_unlock;
	}

	ret = amfi_process_result(result);

leave_unlock:
	amfi_unlock(client);
	if (result) {
		plist_free(result);
	}
	return ret;
}