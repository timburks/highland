#import <Foundation/Foundation.h>
#import <Nu/Nu.h>
#import <highlander.h>

#define VERBOSE

#define DECLINED 0
#define OK 1

@class Highland;

Highland *highland;                               // Because we don't construct handler function pointers at runtime, we allow just one highland.
Class NuCell;					  // Nu cons cell object
Class NuRegex;					  // Nu regular expressions
Class NuSymbolTable;				  // Nu symbol table class
id REQUEST;                                       // symbol
id RESPONSE;                                      // symbol

@interface HighlandRequest : NSObject
{
    @public
    http_request request;
    id match;
}

@end

@implementation HighlandRequest

- (id) match {return match;}
- (void) setMatch:(id) m {
    [m retain];
    [match release];
    match = m;
}

- (NSDictionary *) parameters
{
    NSMutableDictionary *result = [NSMutableDictionary dictionary];
    int count = request_get_parameter_count(request);
    for (int i = 0; i < count; i++) {
        char *name = request_get_parameter_name(request, i);
        char *value = request_get_parameter_value(request, name);
        [result setObject:[[[NSString alloc] initWithCString:value encoding:NSUTF8StringEncoding] autorelease]
            forKey:[[[NSString alloc] initWithCString:name encoding:NSUTF8StringEncoding] autorelease]];
    }
    return result;
}

- (NSDictionary *) postDictionary
{
    NSMutableDictionary *result = [NSMutableDictionary dictionary];
    int count = request_get_field_count(request);
    for (int i = 0; i < count; i++) {
        int namelen = request_get_field_namelen(request, i);
        int valuelen = request_get_field_valuelen(request, i);
        char *name = (char *) malloc(namelen+1);
        char *value = (char *) malloc(valuelen+1);
        request_get_field_name(request, i, name, namelen);
        request_get_field_value(request, i, value, valuelen);
        name[namelen]=0;
        value[valuelen]=0;
        [result setObject:[[[NSString alloc] initWithCString:value encoding:NSUTF8StringEncoding] autorelease]
            forKey:[[[NSString alloc] initWithCString:name encoding:NSUTF8StringEncoding] autorelease]];
        free(name);
        free(value);
    }
    return result;
}

- (NSData *) postData
{
    return [NSData dataWithBytes:request_get_content(request) length:request_get_content_length(request)];
}

- (NSArray *) cookies
{
    NSMutableArray *result = [NSMutableArray array];
    int count = request_get_cookie_count(request);
    for (int i = 0; i < count; i++) {
        cookie c = request_get_cookie(request, i);
        [result addObject:
        [NSDictionary dictionaryWithObjectsAndKeys:
        [[[NSString alloc] initWithCString:cookie_get_name(c)] autorelease], @"name",
            [[[NSString alloc] initWithCString:cookie_get_value(c)] autorelease], @"value",
            nil]];
    }
    return result;
}

- (NSString *) useragent 
{
    return [NSString stringWithCString:request_get_user_agent(request) encoding:NSUTF8StringEncoding];
}

- (void) dealloc
{
    [match release];
    [super dealloc];
}

@end

@interface HighlandResponse : NSObject
{
    @public
    http_response response;
}

@end

@implementation HighlandResponse

- (void) writeString:(NSString *) string
{
    response_add(response, [string cStringUsingEncoding:NSUTF8StringEncoding]);
}

- (void) setExpiration:(int) value
{
    entity_header eh = response_get_entity_header(response);
    void entity_header_set_expires(entity_header eh, time_t value);
    entity_header_set_expires(eh, value);
}

- (void) setContentType:(NSString *) string
{
    response_set_content_type(response, [string cStringUsingEncoding:NSUTF8StringEncoding]);
}

- (void) setCacheControl:(NSString *) string
{
    response_set_cache_control(response, [string cStringUsingEncoding:NSUTF8StringEncoding]);
}

@end

@interface HighlandHandler : NSObject
{
    http_method method;
    id pattern;
    id handler;
}

- (int) handleRequest:(HighlandRequest *) request withResponse:(HighlandResponse *) response;
@end

@implementation HighlandHandler

- (HighlandHandler *) initWithMethod:(http_method)m pattern:(id) p handler:(id) h
{
    [super init];
    method = m;
    pattern = [p retain];
    handler = [h retain];
    return self;
}

+ (HighlandHandler *) getWithPattern:(id) p handler:(id) h
{
    return [[[self alloc] initWithMethod:METHOD_GET pattern:p handler:h] autorelease];
}

+ (HighlandHandler *) postWithPattern:(id) p handler:(id) h
{
    return [[[self alloc] initWithMethod:METHOD_POST pattern:p handler:h] autorelease];
}

- (int) handleRequest:(HighlandRequest *) request withResponse:(HighlandResponse *) response
{
    // confirm the method
    http_method request_method = request_get_method(request->request);
    if ((request_method == METHOD_GET) || (request_method == METHOD_HEAD)) {
        if (method != METHOD_GET)
            return DECLINED;
    }
    else {
        if (method != METHOD_POST)
            return DECLINED;
    }
    // confirm the path
    NSString *path = [[NSString alloc] initWithCString:request_get_uri(request->request) encoding:NSUTF8StringEncoding];
    if ([pattern isKindOfClass:[NSString class]]) {
        if (![pattern isEqualToString:path])
            return DECLINED;
    }
    else if ([pattern isKindOfClass:[NuRegex class]]) {
        id match = [pattern findInString:path];
        if (!match || (![[match group] isEqualToString:path])) {
            return DECLINED;
        } else {
            [request setMatch:[pattern findInString:path]];
        }
    }
    // If we make it this far, we have a match and should call the handler.
    // We expect the handler to be a list (of NuCells).
    if (!handler || ![handler isKindOfClass:[NuCell class]]) {
        http_response page = response->response;
        response_add(page, "<html>\n<head><title>highland</title></head>\n");
        response_add(page, "<body>\n");
        response_add(page, "<h1>Error</h1>\n");
        response_add(page, "There is no handler for this URL.</body></html>\n");
    }
    else {
        NSMutableDictionary *context = [NSMutableDictionary dictionary];
        [context setObject:request forKey:REQUEST];
        [context setObject:response forKey:RESPONSE];
        [context setObject:[NuSymbolTable sharedSymbolTable] forKey:@"symbols"];
// NS_DURING
@try {
        id result = [handler evalWithContext:context];
	if ([result isKindOfClass:[NSString class]]) 
	    [response writeString:result];
} @catch (id exception) {
//NS_HANDLER
//id exception = localException;
NSLog(@"error: %@ %@", [exception name], [exception reason]);
        http_response page = response->response;
        //response_add(page, "<html><head><title>Server error</title></head><body>\n");
        response_add(page, "<h1>Server Error</h1>\n");
        response_add(page, "<h2>");
        response_add(page, [[exception name] cStringUsingEncoding:NSUTF8StringEncoding]);
        response_add(page, "</h2>\n");
        response_add(page, "<p>");
        response_add(page, [[exception reason] cStringUsingEncoding:NSUTF8StringEncoding]);
        response_add(page, "</p>\n");
        //response_add(page, "</body></html>\n");
    }
//NS_ENDHANDLER
    }
    return OK;
}

@end

@interface Highland : NSObject
{
    http_server s;
    NSMutableArray *handlers;
}

- (int) handleRequest:(http_request) req withResponse:(http_response) page;

@end

@implementation Highland

- (int) handleRequest:(http_request) req withResponse:(http_response) page
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

#ifdef VERBOSE
    printf("uri %s\n", request_get_uri(req));
    printf("host %s\n", request_get_host(req));
    int count = request_get_parameter_count(req);
    printf("parameter count %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("%s = %s\n", request_get_parameter_name(req, i), request_get_parameter_value(req, request_get_parameter_name (req, i)));
    }
    count = request_get_cookie_count(req);
    printf("cookie count %d\n", count);
    for (int i = 0; i < count; i++) {
        cookie c = request_get_cookie(req, i);
        printf("%s %s\n", cookie_get_name(c), cookie_get_value(c));
    }
    count = request_get_field_count(req);
    printf("field count %d\n", count);
    for (int i = 0; i < count; i++) {
        int namelen = request_get_field_namelen(req, i);
        int valuelen = request_get_field_valuelen(req, i);
        char *name = (char *) malloc(namelen+1);
        char *value = (char *) malloc(valuelen+1);
        request_get_field_name(req, i, name, namelen);
        request_get_field_value(req, i, value, valuelen);
        name[namelen]=0;
        value[valuelen]=0;
        printf("%s = %s\n", name, value);
        free(name);
        free(value);
    }
#endif

    char handled = NO;
    int status;
    HighlandRequest *request = [[HighlandRequest alloc] init];
    HighlandResponse *response = [[HighlandResponse alloc] init];
    request->request = req;
    response->response = page;
    for (int i = 0; i < [handlers count]; i++) {
        HighlandHandler *handler = [handlers objectAtIndex:i];
        if ([handler handleRequest:request withResponse:response]) {
            handled = YES;
            status = HTTP_200_OK;
            break;
        }
    }
    [request release];
    [response release];
    if (!handled) {
        response_add(page, "<html>\n<head><title>404 Page Not Found</title></head>\n");
        response_add(page, "<body>\n");
        response_add(page, "<h1>Resource not found.</h1>\n");
        response_add(page, "</body></html>\n");
        response_set_status(page, HTTP_404_NOT_FOUND);
        status = HTTP_404_NOT_FOUND;
        status = 0;
    }
    [pool release];
    return status;
}

int highland_handler(http_request req, http_response page)
{
    return [highland handleRequest:req withResponse:page];
}

- (Highland *) initWithPort:(int) port
{
    [super init];
    s = http_server_new();
    http_server_set_port(s, port);
    http_server_alloc(s);
    http_server_set_default_page_handler(s, highland_handler);
    http_server_set_documentroot(s, "public");
    http_server_set_can_read_files(s, 1);
    // all page handlers go here
    handlers = [[NSMutableArray alloc] init];
    // currently we support at most one highland, as pointed by this global variable
    highland = self;
    // get these symbols
    NuCell = NSClassFromString(@"NuCell");
    NuRegex = NSClassFromString(@"NuRegex");
    NuSymbolTable = NSClassFromString(@"NuSymbolTable");
    REQUEST = [[NuSymbolTable sharedSymbolTable] symbolWithCString:"REQUEST"];
    RESPONSE = [[NuSymbolTable sharedSymbolTable] symbolWithCString:"RESPONSE"];
    return self;
}

- (void) addHandler:(HighlandHandler *) handler
{
    [handlers addObject:handler];
}

- (void) start
{
    http_server_get_root_resources(s);
    http_server_start(s);
}

@end

@interface DumbTest : NSObject {} 
@end

@implementation DumbTest
+ (void) run {
   NSData *d = [@"foo" dataUsingEncoding:4];
   NSString *s = [[NSString alloc] initWithData:d encoding:4];
   NSLog(@"testing... %@", s);
}
@end

@implementation NSString (more)
+ (id) stringWithData:(NSData *)data encoding:(int) encoding
{
   return [[[NSString alloc] initWithData:data encoding:encoding] autorelease];
}
@end
