
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
#include "lockdown.h"
#include "mobile_image_mounter.h"
#include "common/userpref.h"
#include "libimobiledevice-glue/socket.h"
#include "libimobiledevice-glue/thread.h"
#include "common/debug.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
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

LIBIMOBILEDEVICE_API idevice_error_t idevice_new_force_network_ipv6(idevice_t * device, const char *udid, const char* host)
{
	idevice_t _device = (idevice_t)malloc(sizeof(struct idevice_private));
	if (!_device)
		return IDEVICE_E_NO_DEVICE;

	struct sockaddr_in6 sa; 
	memset(&sa, 0, sizeof(sa));
	sa.sin6_family = AF_INET6;
	sa.sin6_port = 0; // htons(port);
	inet_pton(AF_INET6, host, &sa.sin6_addr); 

	_device->udid = strdup(udid);
	_device->mux_id = 19900724;
	_device->version = 0;
	_device->conn_type = CONNECTION_NETWORK;
	_device->conn_data = malloc(sizeof(sa));
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

LIBIMOBILEDEVICE_API lockdownd_error_t lockdownd_client_new_with_rsd(idevice_t device, lockdownd_client_t *client, const char *label, int lockdown_port)
{
	if (!device || !client)
		return LOCKDOWN_E_INVALID_ARG;

	struct lockdownd_service_descriptor service;
	memset(&service, 0, sizeof(struct lockdownd_service_descriptor));
	service.port = lockdown_port;
	service.ssl_enabled = 0;

	property_list_service_client_t plistclient = NULL;
	if (property_list_service_client_new(device, (lockdownd_service_descriptor_t)&service, &plistclient) != PROPERTY_LIST_SERVICE_E_SUCCESS) {
		debug_info("could not connect to lockdownd (device %s)", device->udid);
		return LOCKDOWN_E_MUX_ERROR;
	}

	lockdownd_client_t client_loc = (lockdownd_client_t) malloc(sizeof(struct lockdownd_client_private));
	client_loc->parent = plistclient;
	client_loc->ssl_enabled = 0;
	client_loc->session_id = NULL;
	client_loc->device = device;
	client_loc->cu_key = NULL;
	client_loc->cu_key_len = 0;

	if (device->udid) {
		debug_info("device udid: %s", device->udid);
	}

	client_loc->label = label ? strdup(label) : NULL;

	*client = client_loc;

	return LOCKDOWN_E_SUCCESS;
}

LIBIMOBILEDEVICE_API lockdownd_error_t lockdownd_client_new_with_rsd_checkin(idevice_t device, lockdownd_client_t *client, const char *label, int lockdown_port)
{
	if (!client)
		return LOCKDOWN_E_INVALID_ARG;

	lockdownd_error_t ret = LOCKDOWN_E_SUCCESS;
	lockdownd_client_t client_loc = NULL;
	plist_t pair_record = NULL;
	char *host_id = NULL;
	char *type = NULL;

	ret = lockdownd_client_new_with_rsd(device, &client_loc, label, lockdown_port);
	if (LOCKDOWN_E_SUCCESS != ret) {
		debug_info("failed to create lockdownd client.");
		return ret;
	}

	plist_t dict = plist_new_dict();
	plist_dict_set_item(dict, "Label", plist_new_string(label));
	plist_dict_set_item(dict, "Request", plist_new_string("RSDCheckin"));
	plist_dict_set_item(dict, "ProtocolVersion", plist_new_string(LOCKDOWN_PROTOCOL_VERSION));
	plist_print(dict);

	ret = lockdownd_send(client_loc, dict);
	plist_free(dict);
	dict = NULL;
	if (LOCKDOWN_E_SUCCESS != ret)
		return ret;

	ret = lockdownd_receive(client_loc, &dict);
	if (LOCKDOWN_E_SUCCESS != ret)
		return ret;
	plist_print(dict); // { "Request": "RSDCheckin" }
	plist_t request = plist_dict_get_item(dict, "Request");
	if (request == NULL || plist_string_val_compare(request, "RSDCheckin")) {
		return LOCKDOWN_E_UNKNOWN_ERROR;
	}
	plist_free(dict);
	dict = NULL;

	ret = lockdownd_receive(client_loc, &dict);
	if (LOCKDOWN_E_SUCCESS != ret)
		return ret;
	plist_print(dict); // { "Request": "StartService" }
	request = plist_dict_get_item(dict, "Request");
	if (request == NULL || plist_string_val_compare(request, "StartService")) {
		return LOCKDOWN_E_UNKNOWN_ERROR;
	}
	plist_free(dict);
	dict = NULL;

	*client = client_loc;
	return LOCKDOWN_E_SUCCESS;
}

LIBIMOBILEDEVICE_API service_error_t service_client_factory_start_service_with_rsd(idevice_t device, const char* service_name, int service_port, void **client, const char* label, int lockdown_port, int32_t (*constructor_func)(idevice_t, lockdownd_service_descriptor_t, void**), int32_t *error_code) 
{
	*client = NULL;

	lockdownd_client_t lckd = NULL;
	if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_rsd(device, &lckd, label, lockdown_port)) {
		debug_info("Could not create a lockdown client.");
		return SERVICE_E_START_SERVICE_ERROR;
	}

	lockdownd_service_descriptor_t service = (lockdownd_service_descriptor_t)malloc(sizeof(struct lockdownd_service_descriptor));
	service->port = service_port;
	service->ssl_enabled = 0;
	service->identifier = strdup(service_name);

	int32_t ec;
	if (constructor_func) {
		ec = (int32_t)constructor_func(device, service, client);
	} else {
		ec = service_client_new(device, service, (service_client_t*)client);
	}
	if (error_code) {
		*error_code = ec;
	}

	if (ec != SERVICE_E_SUCCESS) {
		debug_info("Could not connect to service %s! Port: %i, error: %i", service_name, service->port, ec);
	}

	lockdownd_service_descriptor_free(service);
	service = NULL;

	return (ec == SERVICE_E_SUCCESS) ? SERVICE_E_SUCCESS : SERVICE_E_START_SERVICE_ERROR;
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

LIBIMOBILEDEVICE_API instrument_error_t instrument_client_start_service_with_rsd(idevice_t device, instrument_client_t* client, const char* label, int lockdown_port, int service_port){
    // internal_set_debug_level(1);
    instrument_error_t err = INSTRUMENT_E_UNKNOWN_ERROR;
    instrument_error_t start_service_error = INSTRUMENT_E_UNKNOWN_ERROR;

	// try rsd mode first
	if (lockdown_port > 0 && service_port > 0) {
		start_service_error = instrument_error(service_client_factory_start_service_with_rsd(device, INSTRUMENT_REMOTESERVER_SERVICE_NAME_WITH_RSD, service_port, (void**)client, label, lockdown_port, SERVICE_CONSTRUCTOR(instrument_client_new), &err));
		if (start_service_error == INSTRUMENT_E_SUCCESS){
			return err;
		}
	}
	
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
	uint64_t strval_b = 0;
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
	if (node) {
		if (plist_get_node_type(node) == PLIST_BOOLEAN) {
			plist_get_bool_val(node, &strval_b);
		} else if (plist_get_node_type(node) == PLIST_INT) {
			plist_get_uint_val(node, &strval_b);
		}
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