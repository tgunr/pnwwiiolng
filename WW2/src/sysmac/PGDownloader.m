/* $Id: PGDownloader.m 2022 2009-10-26 02:28:06Z ahwulf@CRS $ */
#import "PGDownloader.h"

@interface PGDownloader (PGDownloaderPrivate)
- (void)setFile:(NSString*)newFile;
- (void)setServer:(NSString*)newServer;
- (void)setProtocol:(NSString*)newProtocol;
- (void)setCredentials:(NSArray*)newCredentials;
- (void)setDirectory:(NSString*)newDirectory;

- (NSURLResponse*)response;
- (void)setResponse:(NSURLResponse*)newDownloadResponse;
- (void)setDownloadedFilePath:(NSString*)newDownloadedFilePath;
@end

@implementation PGDownloader

+ downloaderForFile:(NSString*)inFile
             server:(NSString*)inServer
           protocol:(NSString*)inProtocol 
        credentials:(NSArray*)inCredentials
        toDirectory:(NSString*)inDirectory
{
    PGDownloader* downloader = [[PGDownloader alloc] init];
    if (downloader == nil) return nil;
    
    [downloader setFile:inFile];
    [downloader setServer:inServer];
    [downloader setProtocol:inProtocol];
    [downloader setCredentials:inCredentials];
    [downloader setDirectory:[inDirectory stringByStandardizingPath]]; 
        
    return [downloader autorelease];
}


- (void)dealloc
{
    [self setResponse:nil];
    [self setDownloadedFilePath:nil];
    
    [self setFile:nil];
    [self setServer:nil];
    [self setProtocol:nil];
    [self setCredentials:nil];
    [self setDirectory:nil];
    
    [self setDelagate:nil];
  [super dealloc];
}

- (NSString *)file {
    return file;
}

- (void)setFile:(NSString *)newFile {
    if (file != newFile) {
        [file release];
        file = [newFile retain];
    }
}

- (NSString *)server {
    return server;
}

- (void)setServer:(NSString *)newServer {
    if (server != newServer) {
        [server release];
        server = [newServer retain];
    }
}

- (NSString *)protocol {
    return protocol;
}

- (void)setProtocol:(NSString *)newProtocol {
    if (protocol != newProtocol) {
        [protocol release];
        protocol = [newProtocol retain];
    }
}

- (NSArray *)credentials {
    return credentials;
}

- (void)setCredentials:(NSArray *)newCredentials {
    if (credentials != newCredentials) {
        [credentials release];
        credentials = [newCredentials retain];
    }
}

- (NSString *)directory {
    return directory;
}

- (void)setDirectory:(NSString *)newDirectory {
    if (directory != newDirectory) {
        [directory release];
        directory = [newDirectory retain];
    }
}

- (NSURLResponse *)response {
    return response;
}

- (void)setResponse:(NSURLResponse *)newResponse {
    if (response != newResponse) {
        [response release];
        response = [newResponse retain];
    }
}

- (NSString *)downloadedFilePath {
    return downloadedFilePath;
}

- (void)setDownloadedFilePath:(NSString *)newDownloadedFilePath {
    if (downloadedFilePath != newDownloadedFilePath) {
        [downloadedFilePath release];
        downloadedFilePath = [newDownloadedFilePath retain];
    }
}

- (id)delagate {
    return delagate;
}

- (void)setDelagate:(id)newDelagate {
    if (delagate != newDelagate) {
        [delagate release];
        delagate = [newDelagate retain];
    }
}

- (void)startDownload
{
    NSURL* url = [[[NSURL alloc] initWithScheme:[self protocol]
                                           host:[self server]
                                           path:[NSString stringWithFormat:@"/%@", [self file]]] autorelease];
    
    NSURLRequest *request = [NSURLRequest requestWithURL:url
                                             cachePolicy:NSURLRequestReloadIgnoringCacheData
                                         timeoutInterval:60.0];
    
    urld = [[NSURLDownload alloc] initWithRequest:request delegate:self];
    if (urld != nil) {
        [self retain];
    } else {
        if ([delagate respondsToSelector:@selector(downloader:didFailWithError:)]) {
            [delagate downloader:self didFailWithError:nil]; //TODO
        }
    }
}

- (void)abortDownload
{
    if (urld != nil) {
        [urld cancel];
        [urld release];
        urld = nil;
        
        [self release];
    }
}

- (void)download:(NSURLDownload*)download didFailWithError:(NSError*)error
{
    [download release];
    
    if ([delagate respondsToSelector:@selector(downloader:didFailWithError:)]) {
        [delagate downloader:self didFailWithError:error];
    }

    [self release];
}

- (void)downloadDidFinish:(NSURLDownload *)download
{
    [download release];
    
    if ([delagate respondsToSelector:@selector(downloader:didDownloadFile:)]) {
        [delagate downloader:self didDownloadFile:[self downloadedFilePath]];
    }
    
    [self release];
}

- (void)download:(NSURLDownload*)download decideDestinationWithSuggestedFilename:(NSString *)filename
{
    [self setDownloadedFilePath:[NSString stringWithFormat:@"%@/%@", [self directory], filename]];
    [download setDestination:[self downloadedFilePath] allowOverwrite:YES];
}

- (BOOL)download:(NSURLDownload*)download shouldDecodeSourceDataOfMIMEType:(NSString*)encodingType
{    
    return YES; // have it decode all supported formats for us automatically
}

-(void)download:(NSURLDownload*)download didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge*)challenge
{
    NSArray* creds = [self credentials];
    
    unsigned int failureCount = [challenge previousFailureCount];    
    if (failureCount < [creds count]) {
        [[challenge sender] useCredential:[creds objectAtIndex:failureCount] 
               forAuthenticationChallenge:challenge];
        
    }
}

- (void)download:(NSURLDownload*)download didReceiveResponse:(NSURLResponse*)inResponse
{
    bytesReceived = 0;
    [self setResponse:inResponse];
    [self download:download didReceiveDataOfLength:0];
    
    if ([delagate respondsToSelector:@selector(downloaderDidStart:)]) {
        [delagate downloaderDidStart:self];
    }
}

- (void)downloadDidBegin
{
    if ([delagate respondsToSelector:@selector(downloaderDidStart:)]) {
        [delagate downloaderDidStart:self];
    }
}

- (void)download:(NSURLDownload*)download didReceiveDataOfLength:(unsigned)length
{
    bytesReceived = bytesReceived + length;
    
    long long expectedLength = [[self response] expectedContentLength]; 
    if (expectedLength != NSURLResponseUnknownLength) {        
        if ([delagate respondsToSelector:@selector(downloader:bytesReceived:outOf:)]) {
            [delagate downloader:self bytesReceived:bytesReceived outOf:expectedLength];
        }
    } else {        
        if ([delagate respondsToSelector:@selector(downloader:bytesReceived:)]) {
            [delagate downloader:self bytesReceived:bytesReceived];
        }
    }
}
@end
