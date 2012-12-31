//
//  PGDownloader.h
//  NewPlayNet
//
//  Created by crs on 12/9/04.
//  Copyright 2004 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PGDownloader : NSObject {
@private
    NSString* file;
    NSString* server;
    NSString* protocol;
    NSArray* credentials;
    NSString* directory;

    id delagate;

    NSURLDownload* urld;
    NSURLResponse* response;
    NSString* downloadedFilePath;
    long long bytesReceived;
}

+ downloaderForFile:(NSString*)inFile
             server:(NSString*)inServer
           protocol:(NSString*)inProtocol
        credentials:(NSArray*)inCredentials
        toDirectory:(NSString*)inDirectory;

- (NSString*)downloadedFilePath;

- (NSString*)file;
- (NSString*)server;
- (NSString*)protocol;
- (NSArray*)credentials;
- (NSString*)directory;

- (id)delagate;
- (void)setDelagate:(id)newDelagate;

- (void)startDownload;
- (void)abortDownload;
@end

@interface NSObject (PGDownloaderDelagate)
- (void)downloaderDidStart:(PGDownloader*)downloader;
- (void)downloader:(PGDownloader*)downloader didDownloadFile:(NSString*)filePath;
- (void)downloader:(PGDownloader*)downloader didFailWithError:(NSError*)error;
- (void)downloader:(PGDownloader*)downloader bytesReceived:(long long)bytesReceived outOf:(long long)expectedLength;
- (void)downloader:(PGDownloader*)downloader bytesReceived:(long long)bytesReceived;
@end
