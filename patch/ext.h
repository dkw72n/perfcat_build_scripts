#ifndef IMOBILEDEVICE_EXT_H
#define IMOBILEDEVICE_EXT_H
#include "libimobiledevice.h"
#include "mobile_image_mounter.h"

#ifdef __cplusplus
extern "C" {
#endif

idevice_error_t idevice_new_force_network(idevice_t * device, const char *udid, const char* host);
mobile_image_mounter_error_t mobile_image_mounter_upload_image_file(mobile_image_mounter_client_t client, const char *image_type, const char* image_file_path, const char *signature_file_path);
mobile_image_mounter_error_t mobile_image_mounter_mount_image_file(mobile_image_mounter_client_t client, const char *image_path, const char *signature_file, const char *image_type, plist_t *result);
void libimobiledevice_free(void* ptr);

#define INSTRUMENT_REMOTESERVER_SERVICE_NAME "com.apple.instruments.remoteserver"
#define INSTRUMENT_REMOTESERVER_SECURE_SERVICE_NAME INSTRUMENT_REMOTESERVER_SERVICE_NAME ".DVTSecureSocketProxy"

/** Error Codes */
typedef enum {
	INSTRUMENT_E_SUCCESS         =  0,
	INSTRUMENT_E_INVALID_ARG     = -1,
	INSTRUMENT_E_PLIST_ERROR     = -2,
	INSTRUMENT_E_MUX_ERROR       = -3,
	INSTRUMENT_E_SSL_ERROR       = -4,
	INSTRUMENT_E_RECEIVE_TIMEOUT = -5,
	INSTRUMENT_E_BAD_VERSION     = -6,
    INSTRUMENT_E_CONN_FAILED     = -7,
	INSTRUMENT_E_UNKNOWN_ERROR   = -256
} instrument_error_t;

typedef struct instrument_client_private instrument_client_private;
typedef instrument_client_private *instrument_client_t; /**< The client handle. */

/**
 * Connects to the instrument service on the specified device.
 *
 * @param device The device to connect to.
 * @param service The service descriptor returned by lockdownd_start_service.
 * @param client Pointer that will be set to a newly allocated
 *     instrument_client_t upon successful return.
 *
 * @note This service is only available if a developer disk image has been
 *     mounted.
 *
 * @return INSTRUMENT_E_SUCCESS on success, INSTRUMENT_E_INVALID_ARG if one
 *     or more parameters are invalid, or INSTRUMENT_E_CONN_FAILED if the
 *     connection to the device could not be established.
 */
instrument_error_t instrument_client_new(idevice_t device, lockdownd_service_descriptor_t service, instrument_client_t * client);

/**
 * Starts a new instrument service on the specified device and connects to it.
 *
 * @param device The device to connect to.
 * @param client Pointer that will point to a newly allocated
 *     instrument_client_t upon successful return. Must be freed using
 *     instrument_client_free() after use.
 * @param label The label to use for communication. Usually the program name.
 *  Pass NULL to disable sending the label in requests to lockdownd.
 *
 * @return INSTRUMENT_E_SUCCESS on success, or an INSTRUMENT_E_* error
 *     code otherwise.
 */
instrument_error_t instrument_client_start_service(idevice_t device, instrument_client_t* client, const char* label);

/**
 * Disconnects a instrument client from the device and frees up the
 * instrument client data.
 *
 * @param client The instrument client to disconnect and free.
 *
 * @return INSTRUMENT_E_SUCCESS on success, or INSTRUMENT_E_INVALID_ARG
 *     if client is NULL.
 */
instrument_error_t instrument_client_free(instrument_client_t client);

/**
 * Send command to the connected device.
 *
 * @param client The connection instrument service client.
 * @param data Data to send
 * @param size Size of the Data to send
 * @param sent Number of bytes sent (can be NULL to ignore)
 * @return INSTRUMENT_E_SUCCESS on success, or an INSTRUMENT_E_* error
 *     code otherwise.
 */
instrument_error_t instrument_send_command(instrument_client_t client, const char *data, uint32_t size, uint32_t *sent);

/**
 * Receives data using the given instrument client with specified timeout.
 *
 * @param client The instrument client to use for receiving
 * @param data Buffer that will be filled with the data received
 * @param size Number of bytes to receive
 * @param received Number of bytes received (can be NULL to ignore)
 * @param timeout Maximum time in milliseconds to wait for data.
 *
 * @return INSTRUMENT_E_SUCCESS on success,
 *      INSTRUMENT_E_INVALID_ARG when one or more parameters are
 *      invalid, INSTRUMENT_E_MUX_ERROR when a communication error
 *      occurs, or INSTRUMENT_E_UNKNOWN_ERROR when an unspecified
 *      error occurs.
 */
instrument_error_t instrument_receive_with_timeout(instrument_client_t client, char *data, uint32_t size, uint32_t *received, unsigned int timeout);

/**
 * Receives data using the given instrument client.
 *
 * @param client The instrument client to use for receiving
 * @param data Buffer that will be filled with the data received
 * @param size Number of bytes to receive
 * @param received Number of bytes received (can be NULL to ignore)
 *
 * @return INSTRUMENT_E_SUCCESS on success,
 *      INSTRUMENT_E_INVALID_ARG when one or more parameters are
 *      invalid, INSTRUMENT_E_NOT_ENOUGH_DATA when not enough data
 *      received, INSTRUMENT_E_TIMEOUT when the connection times out,
 *      INSTRUMENT_E_MUX_ERROR when a communication error
 *      occurs, or INSTRUMENT_E_UNKNOWN_ERROR when an unspecified
 *      error occurs.
 */
instrument_error_t instrument_receive(instrument_client_t client, char *data, uint32_t size, uint32_t *received);

void ljj_preflight(idevice_t * device);

#define AMFI_REMOTESERVER_SERVICE_NAME "com.apple.amfi.lockdown"

/** Error Codes */
typedef enum {
	AMFI_E_SUCCESS         =  0,
	AMFI_E_INVALID_ARG     = -1,
	AMFI_E_PLIST_ERROR     = -2,
	AMFI_E_CONN_FAILED     = -3,
	AMFI_E_COMMAND_FAILED  = -4,
	AMFI_E_DEVICE_LOCKED   = -5,
	AMFI_E_DEVICE_HAS_A_PASSCODE_SET = -6,
	AMFI_E_UNKNOWN_ERROR   = -256

} amfi_error_t;

typedef struct amfi_client_private amfi_client_private;
typedef amfi_client_private *amfi_client_t; /**< The client handle. */

/**
 * Connects to the amfi service on the specified device.
 *
 * @param device The device to connect to.
 * @param service The service descriptor returned by lockdownd_start_service.
 * @param client Pointer that will be set to a newly allocated
 *     amfi_client_t upon successful return.
 *
 * @note This service is only available if a developer disk image has been
 *     mounted.
 *
 * @return AMFI_E_SUCCESS on success, AMFI_E_INVALID_ARG if one
 *     or more parameters are invalid.
 */
amfi_error_t amfi_client_new(idevice_t device, lockdownd_service_descriptor_t service, amfi_client_t * client);

/**
 * Starts a new instrument service on the specified device and connects to it.
 *
 * @param device The device to connect to.
 * @param client Pointer that will point to a newly allocated
 *     amfi_client_t upon successful return. Must be freed using
 *     amfi_client_free() after use.
 * @param label The label to use for communication. Usually the program name.
 *  Pass NULL to disable sending the label in requests to lockdownd.
 *
 * @return AMFI_E_SUCCESS on success, or an AMFI_E_* error
 *     code otherwise.
 */
amfi_error_t amfi_client_start_service(idevice_t device, amfi_client_t* client, const char* label);

/**
 * Disconnects an amfi client from the device and frees up the
 * amfi client data.
 *
 * @param client The amfi client to disconnect and free.
 *
 * @return AMFI_E_SUCCESS on success, or AMFI_E_INVALID_ARG
 *     if client is NULL.
 */
amfi_error_t amfi_client_free(amfi_client_t client);

/**
 * Active the developer mode on device.
 * 
 * @param client The connected device.
 * @param mode The argument mode include one of the fllowing mode: 0, 1, 
 * 		which represents "close the developer mode" 
 * 		and "active the developer mode" respectively.
 * @return AMFI_E_SUCCESS on success, or an AMFI_E_* error
 * 		code otherwise.
 */
amfi_error_t amfi_set_developer_mode(amfi_client_t client, int mode);

#ifdef __cplusplus
}
#endif

#endif
