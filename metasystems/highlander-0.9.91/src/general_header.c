#include <stdlib.h>
#include <errno.h>
#include <highlander.h>
#include <internals.h>

typedef unsigned long flagtype;
static int general_header_flag_is_set(general_header gh, flagtype flag);
static void general_header_set_flag(general_header gh, flagtype flag);
static void general_header_clear_flags(general_header gh);
#define GENERAL_HEADER_NO_CACHE_SET				( 0x01)
#define GENERAL_HEADER_NO_STORE_SET				( 0x02)
#define GENERAL_HEADER_MAX_AGE_SET				( 0x04)
#define GENERAL_HEADER_MAX_STALE_SET			( 0x08)
#define GENERAL_HEADER_MIN_FRESH_SET			( 0x10)
#define GENERAL_HEADER_NO_TRANSFORM_SET			( 0x20)
#define GENERAL_HEADER_DATE_SET					( 0x40)
#define GENERAL_HEADER_TRAILER_SET				( 0x80)
#define GENERAL_HEADER_TRANSFER_ENCODING_SET	( 0x100)
#define GENERAL_HEADER_ONLY_IF_CACHED_SET		( 0x200)
#define GENERAL_HEADER_UPGRADE_SET				( 0x400)
#define GENERAL_HEADER_VIA_SET					( 0x800)
#define GENERAL_HEADER_WARNING_SET				( 0x1000)
#define GENERAL_HEADER_CONNECTION_SET			( 0x2000)
#define GENERAL_HEADER_PRAGMA_SET				( 0x4000)

struct general_header_tag {
	flagtype flags; /* See http_request.c for an explanation */

	/* Cache control flags/values , v1.1 §14.9 */
	int no_cache;				/* v1.0 §10.12 v1.1 §14.32 */
	int no_store;
	int max_age;
	int max_stale;
	int min_fresh;
	int no_transform;
	int only_if_cached;

	cstring connection;			/* v1.1 §14.10 */
	time_t date;				/* v1.0 §10.6 v1.1 §14.18 */
	cstring pragma;				/* §14.32 */
	cstring trailer;			/* v1.1 §14.40 */
	cstring transfer_encoding;	/* v1.1 §14.41 */
	cstring upgrade;			/* v1.1 §14.42 */
	cstring via;				/* v1.1 §14.45 */
	cstring warning;			/* v1.1 §14.46 */
};


void general_header_free(general_header p)
{
	if(p != NULL) {
		cstring_free(p->connection);
		cstring_free(p->pragma);
		cstring_free(p->trailer);
		cstring_free(p->transfer_encoding);
		cstring_free(p->upgrade);
		cstring_free(p->via);
		cstring_free(p->warning);
		free(p);
	}
}

general_header general_header_new(void)
{
	general_header p;

	if( (p = malloc(sizeof *p)) != NULL) {
		general_header_clear_flags(p);
		p->connection = cstring_new();
		p->pragma = cstring_new();
		p->trailer = cstring_new();
		p->transfer_encoding = cstring_new();
		p->upgrade = cstring_new();
		p->via = cstring_new();
		p->warning = cstring_new();
	}

	return p;
}

void general_header_recycle(general_header p)
{
	general_header_clear_flags(p);
}

void general_header_set_date(general_header gh, time_t value) 
{
	assert(NULL != gh);
	assert(value != (time_t)-1);

	gh->date = value;
	general_header_set_flag(gh, GENERAL_HEADER_DATE_SET);
}

int general_header_set_connection(general_header gh, const char* value) 
{
	assert(NULL != gh);
	assert(NULL != value);

	if(!cstring_copy(gh->connection, value)) 
		return 0;

	general_header_set_flag(gh, GENERAL_HEADER_CONNECTION_SET);
	return 1;
}

int general_header_set_pragma(general_header gh, const char* value)
{
	assert(NULL !=  gh);
	assert(NULL != value);

	if(!cstring_copy(gh->pragma, value))
		return 0;

	general_header_set_flag(gh, GENERAL_HEADER_PRAGMA_SET);
	return 1;
}

int general_header_set_trailer(general_header gh, const char* value)
{
	assert(NULL != gh);
	assert(NULL != value);

	if(!cstring_copy(gh->trailer, value)) 
		return 0;

	general_header_set_flag(gh, GENERAL_HEADER_TRAILER_SET);
	return 1;
}

int general_header_set_transfer_encoding(general_header gh, const char* value)
{
	assert(NULL != gh);
	assert(NULL != value);

	if(!cstring_copy(gh->transfer_encoding, value)) 
		return 0;

	general_header_set_flag(gh, GENERAL_HEADER_TRANSFER_ENCODING_SET);
	return 1;
}

int general_header_set_upgrade(general_header gh, const char* value)
{
	assert(NULL != gh);
	assert(NULL != value);

	if(!cstring_copy(gh->upgrade, value)) 
		return 0;

	general_header_set_flag(gh, GENERAL_HEADER_UPGRADE_SET);
	return 1;
}

int general_header_set_via(general_header gh, const char* value)
{
	assert(NULL != gh);
	assert(NULL != value);

	if(!cstring_copy(gh->via, value)) 
		return 0;

	general_header_set_flag(gh, GENERAL_HEADER_VIA_SET);
	return 1;
}

int general_header_set_warning(general_header gh, const char* value)
{
	assert(NULL != gh);
	assert(NULL != value);

	if(!cstring_copy(gh->warning, value)) 
		return 0;

	general_header_set_flag(gh, GENERAL_HEADER_WARNING_SET);
	return 1;
}

void general_header_set_no_cache(general_header gh)
{
	assert(NULL != gh);

	gh->no_cache = 1;
	general_header_set_flag(gh, GENERAL_HEADER_NO_CACHE_SET);
}

void general_header_set_no_store(general_header gh)
{
	assert(NULL != gh);

	gh->no_store = 1;
	general_header_set_flag(gh, GENERAL_HEADER_NO_STORE_SET);
}

void general_header_set_max_age(general_header gh, int value)
{
	assert(NULL != gh);

	gh->max_age = value;
	general_header_set_flag(gh, GENERAL_HEADER_MAX_AGE_SET);
}

void general_header_set_max_stale(general_header gh, int value)
{
	assert(NULL != gh);

	gh->max_stale = value;
	general_header_set_flag(gh, GENERAL_HEADER_MAX_STALE_SET);
}

void general_header_set_min_fresh(general_header gh, int value)
{
	assert(NULL != gh);

	gh->min_fresh = value;
	general_header_set_flag(gh, GENERAL_HEADER_MIN_FRESH_SET);
}

void general_header_set_no_transform(general_header gh)
{
	assert(NULL != gh);

	gh->no_transform = 1;
	general_header_set_flag(gh, GENERAL_HEADER_NO_TRANSFORM_SET);
}

void general_header_set_only_if_cached(general_header gh)
{
	assert(NULL != gh);

	gh->only_if_cached = 1;
	general_header_set_flag(gh, GENERAL_HEADER_ONLY_IF_CACHED_SET);
}

static int general_header_flag_is_set(general_header gh, flagtype flag)
{
	assert(NULL != gh);
	assert(flag > 0);

	return gh->flags & flag ? 1 : 0;
}

static void general_header_set_flag(general_header gh, flagtype flag)
{
	assert(NULL != gh);
	assert(flag > 0);

	gh->flags |= flag;
}

static void general_header_clear_flags(general_header gh)
{
	assert(NULL != gh);
	gh->flags = 0;
}

int general_header_get_no_cache(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_NO_CACHE_SET));
	return gh->no_cache;
}

int general_header_get_no_store(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_NO_STORE_SET));
	return gh->no_store;
}

int general_header_get_max_age(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_MAX_AGE_SET));
	return gh->max_age;
}

int general_header_get_max_stale(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_MAX_STALE_SET));
	return gh->max_stale;
}

int general_header_get_min_fresh(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_MIN_FRESH_SET));
	return gh->min_fresh;
}

int general_header_get_no_transform(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_NO_TRANSFORM_SET));
	return gh->no_transform;
}

int general_header_get_only_if_cached(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_ONLY_IF_CACHED_SET));
	return gh->only_if_cached;
}

time_t general_header_get_date(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_DATE_SET));
	return gh->date;
}

const char* general_header_get_connection(general_header gh)
{
	assert(gh != NULL);
	if(!general_header_flag_is_set(gh, GENERAL_HEADER_CONNECTION_SET))
		return "";
	else
		return c_str(gh->connection);
}

const char* general_header_get_trailer(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_TRAILER_SET));
	return c_str(gh->trailer);
}

const char* general_header_get_transfer_encoding(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_TRANSFER_ENCODING_SET));
	return c_str(gh->transfer_encoding);
}

const char* general_header_get_upgrade(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_UPGRADE_SET));
	return c_str(gh->upgrade);
}

const char* general_header_get_via(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_VIA_SET));
	return c_str(gh->via);
}

const char* general_header_get_warning(general_header gh)
{
	assert(gh != NULL);
	assert(general_header_flag_is_set(gh, GENERAL_HEADER_WARNING_SET));
	return c_str(gh->warning);
}

int general_header_no_cache_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_NO_CACHE_SET);
}

int general_header_no_store_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_NO_STORE_SET);
}

int general_header_max_age_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_MAX_AGE_SET);
}

int general_header_max_stale_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_MAX_STALE_SET);
}

int general_header_min_fresh_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_MIN_FRESH_SET);
}

int general_header_no_transform_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_NO_TRANSFORM_SET);
}

int general_header_only_if_cached_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_ONLY_IF_CACHED_SET);
}

int general_header_date_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_DATE_SET);
}

int general_header_trailer_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_TRAILER_SET);
}

int general_header_transfer_encoding_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_TRANSFER_ENCODING_SET);
}

int general_header_upgrade_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_UPGRADE_SET);
}

int general_header_via_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_VIA_SET);
}

int general_header_warning_isset(general_header gh)
{
	assert(gh != NULL);
	return general_header_flag_is_set(gh, GENERAL_HEADER_WARNING_SET);
}

static inline int send_connection(general_header gh, connection conn)
{
	return http_send_field(conn, "Connection: ", gh->connection);
}

static inline int send_pragma(general_header p, connection conn)
{
	return http_send_field(conn, "Pragma: ", p->pragma);
}

static inline int send_trailer(general_header gh, connection conn)
{
	return http_send_field(conn, "Trailer: ", gh->trailer);
}

static inline int send_transfer_encoding(general_header gh, connection conn)
{
	return http_send_field(conn, "Transfer-Encoding: ", gh->transfer_encoding);
}

static inline int send_upgrade(general_header gh, connection conn)
{
	return http_send_field(conn, "Upgrade: ", gh->upgrade);
}

static inline int send_via(general_header gh, connection conn)
{
	return http_send_field(conn, "Via: ", gh->via);
}

static inline int send_warning(general_header gh, connection conn)
{
	return http_send_field(conn, "Warning: ", gh->warning);
}

static inline int send_date(general_header gh, connection conn)
{
	return http_send_date(conn, "Date: ", gh->date);
}

int general_header_send_fields(general_header gh, connection c)
{
	int success = 1;
	size_t i, nelem;

	static const struct {
		size_t flag;
		int (*func)(general_header, connection);
	} fields[] = {
		{ GENERAL_HEADER_PRAGMA_SET,			send_pragma },
		{ GENERAL_HEADER_DATE_SET,				send_date },
		{ GENERAL_HEADER_CONNECTION_SET,		send_connection },
		{ GENERAL_HEADER_TRAILER_SET,			send_trailer },
		{ GENERAL_HEADER_TRANSFER_ENCODING_SET,	send_transfer_encoding },
		{ GENERAL_HEADER_UPGRADE_SET,			send_upgrade },
		{ GENERAL_HEADER_VIA_SET,				send_via },
		{ GENERAL_HEADER_WARNING_SET,			send_warning }
	};

	
	nelem = sizeof fields / sizeof *fields;
	for(i = 0; i < nelem; i++) {
		if(general_header_flag_is_set(gh, fields[i].flag))
			if( (success = fields[i].func(gh, c)) == 0)
				break;
	}

	return success;
}
/* General header handlers */
static int set_cache_control(general_header gh, const char* s, meta_error e);
static int parse_pragma(general_header gh, const char* value, meta_error e);
static int parse_cache_control(general_header gh, const char* s, meta_error e);
static int parse_date(general_header gh, const char* s, meta_error e);
static int parse_connection(general_header gh, const char* s, meta_error e);
static int parse_trailer(general_header gh, const char* s, meta_error e);
static int parse_transfer_encoding(general_header gh, const char* value, meta_error e);
static int parse_upgrade(general_header gh, const char* s, meta_error e);
static int parse_via(general_header gh, const char* s, meta_error e);
static int parse_warning(general_header gh, const char* s, meta_error e);
static const struct {
	const char* name;
	int (*handler)(general_header gh, const char* value, meta_error e);
} general_header_fields[] = {
	{ "cache-control",		parse_cache_control },
	{ "date",				parse_date },
	{ "pragma",				parse_pragma },
	{ "connection",			parse_connection },
	{ "trailer",			parse_trailer },
	{ "transfer-encoding",	parse_transfer_encoding },
	{ "upgrade",			parse_upgrade },
	{ "via",				parse_via },
	{ "warning",			parse_warning }
};

/* Return an index in the general header array, or -1 if the field was not found. */
int find_general_header(const char* name)
{
	int i, nelem = sizeof general_header_fields / sizeof *general_header_fields;
	for(i = 0; i < nelem; i++) {
		if(strcmp(general_header_fields[i].name, name) == 0)
			return i;
	}

	return -1;
}
int parse_general_header(int idx, general_header gh, const char* value, meta_error e)
{
	assert(idx >= 0);
	assert((size_t)idx < sizeof general_header_fields / sizeof *general_header_fields);

	return general_header_fields[idx].handler(gh, value, e);
}


static int parse_pragma(general_header gh, const char* value, meta_error e)
{
	UNUSED(e);

	/* The only pragma we understand is no-cache */
	if(strstr(value, "no-cache") == value) 
		general_header_set_no_cache(gh);

	/* Silently ignore unknown pragmas */
	return 1;
}

/* Warnings, from 14.46, look like this:
 *	Warning : warn-code SP warn-agent SP warn-text [SP warn-date]
 * We have already parsed the name and value hopefully contains
 * code+agent+text+optional date
 * Do we care? No, not really. Just store the warning.
 * A response may even contain more than one warning. Do we care? We're neither
 * a client nor a proxy ATM, so just store the value.
 */
static int parse_warning(general_header gh, const char* value, meta_error e)
{
	assert(gh != NULL);
	assert(value != NULL);

	if(!general_header_set_warning(gh, value))
		return set_os_error(e, errno);
	return 1;
}

static int parse_cache_control(general_header gh, const char* value, meta_error e)
{
	char* s;

	/* From rfc2616: Legal cache-control directives in a request
    cache-request-directive =
           "no-cache"                          ; Section 14.9.1
         | "no-store"                          ; Section 14.9.2
         | "max-age" "=" delta-seconds         ; Section 14.9.3, 14.9.4
         | "max-stale" [ "=" delta-seconds ]   ; Section 14.9.3
         | "min-fresh" "=" delta-seconds       ; Section 14.9.3
         | "no-transform"                      ; Section 14.9.5
         | "only-if-cached"                    ; Section 14.9.4
         | cache-extension                     ; Section 14.9.6
	*/

	assert(NULL != gh);
	assert(NULL != value);

	/* Loop through the values looking for separating space.
	 * Then look for the actual word
	 */
	for(;*value != '\0';) {
		if( (s = strchr(value, ' ')) == NULL)
			break;

		/* Skip the space */
		s++;
		if(!set_cache_control(gh, s, e))
			return 0;

		value = s + 1;
	}

	/* Remember the last argument */
	return set_cache_control(gh, value, e);
}

static int parse_date(general_header gh, const char* value, meta_error e)
{
	time_t d;
	assert(NULL != gh);
	assert(NULL != value);

	/* Parse date and create a time_t */
	if( (d = parse_rfc822_date(value)) == -1)
		return set_http_error(e, HTTP_400_BAD_REQUEST);
	
	general_header_set_date(gh, d);
	return 1;
}

/*
 * We only accept "close" or "keep-alive". Other values are regarded as invalid.
 * Do we report 400 or do we just ignore the values? We start off beeing strict...
 */
static int parse_connection(general_header gh, const char* value, meta_error e)
{
	assert(NULL != gh);
	assert(NULL != value);

	
	if(strcasecmp(value, "keep-alive") && strcasecmp(value, "close")) 
		return set_http_error(e, HTTP_400_BAD_REQUEST);
	else if(!general_header_set_connection(gh, value))
		return set_os_error(e, errno);
	else
		return 1;
}

static int parse_trailer(general_header gh, const char* value, meta_error e)
{
	assert(NULL != gh);
	assert(NULL != value);

	if(!general_header_set_trailer(gh, value))
		return set_os_error(e, errno);
	return 1;
}

static int parse_upgrade(general_header gh, const char* value, meta_error e)
{
	assert(NULL != gh);
	assert(NULL != value);

	/* Since we only understand http 1.0 and 1.1, I see
	 * no reason whatsoever to support Upgrade. 
	 *
	 * NOTE: Sat Apr 28 18:51:36 CEST 2001
	 * Hmmh, maybe we should? How else do we support SSL/SHTTP?
	 * If we decide to support Upgrade, the proper return status is 
	 * 		101 Switching Protocols
	 */
	if(!general_header_set_upgrade(gh, value))
		return set_os_error(e, errno);
	return 1;
}

static int parse_via(general_header gh, const char* value, meta_error e)
{
	assert(NULL != gh);
	assert(NULL != value);

	/* NOTE: This is incorrect, we may receive multiple Via: 's */
	if(!general_header_set_via(gh, value))
		return set_os_error(e, errno);

	return 1;
}

/* Local helper for parse_cache_control. Needed since 
 * a lot of code is duplicated inside and after the loop
 * The s argument must/should point to a legal request-directive
 * to be understood.
 * Returns 1 if OK, even if the directive wasn't understood.
 * This is to 'accept' extensions from 14.9.6
 */
static int set_cache_control(general_header gh, const char* s, meta_error e)
{
	/*
	 * We have 2 types of cache-request-directives, with and 
	 * without a numeric argument. We ignore extensions for now. 
	 */
	static const struct {
		const char* directive;
		void (*func)(general_header);
	} type1[] = {
		{ "no-cache",		general_header_set_no_cache, },
		{ "no-store",		general_header_set_no_store, },
		{ "no-transform",	general_header_set_no_transform, },
		{ "only-if-cached",	general_header_set_only_if_cached, },
	};

	static const struct {
		const char* directive;
		void (*func)(general_header, int);
	} type2[] = {
		{ "max-age",	general_header_set_max_age, },
		{ "max-stale",	general_header_set_max_stale, },
		{ "min-fresh",	general_header_set_min_fresh, },
	};

	size_t i;

	/* Now look for type1 request-directives */
	for(i = 0; i < sizeof(type1) / sizeof(type1[0]); i++) {
		if(strstr(s, type1[i].directive) == s) {
			/* NOTE: There MAY slip in a bug here, in case
			 * a new directive starts with the same name
			 * as an existing directive. We ignore that now,
			 * then we don't have to extract the name from 
			 * the string 'value'
			 */
			(*type1[i].func)(gh);
			return 1;
		}
	}

	/* Not a type1 directive, try type2 */
	for(i = 0; i < sizeof(type2) / sizeof(type2[0]); i++) {
		if(strstr(s, type2[i].directive) == s) {
			/* NOTE: Same 'bug' as above */
			char *eq = strchr(s, '=');
			int arg;

			/* Could not find = as in NAME=value */
			if(NULL == eq)
				return set_http_error(e, HTTP_400_BAD_REQUEST);

			/* Skip = and convert arg to integer */
			eq++; 
			arg = -1;
			arg = atoi(eq);
			if(-1 == arg) {
				/* Conversion error. NOTE: How about strtol() instead? */
				return set_http_error(e, HTTP_400_BAD_REQUEST);
			}

			/* Call function and continue */
			(*type2[i].func)(gh, arg);
			return 1;
		}
	}

	/* Not found */
	return 1;
}

static int parse_transfer_encoding(general_header gh, const char* value, meta_error e)
{
	if(!general_header_set_transfer_encoding(gh, value))
		return set_os_error(e, errno);

	return 1;
}

int general_header_dump(general_header gh, FILE* f)
{
	(void)gh;(void)f;
	return 1;
}

