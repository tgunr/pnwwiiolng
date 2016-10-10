#import <Cocoa/Cocoa.h>

@class BITFeedbackManager;
@class BITFeedbackMessageAttachment;
@class BITSDKColoredView;
@class BITSDKTextView;
@class QLPreviewPanel;

@interface BITFeedbackWindowController : NSWindowController
{
  __unsafe_unretained BITFeedbackManager *_manager;
  NSDateFormatter *_lastUpdateDateFormatter;
  __unsafe_unretained IBOutlet NSView *_userDataView;
  __unsafe_unretained IBOutlet BITSDKColoredView *_mainBackgroundView;
  __unsafe_unretained IBOutlet BITSDKColoredView *_userDataBoxView;
  __unsafe_unretained IBOutlet BITSDKColoredView *_feedbackListBackgroundView;
  __unsafe_unretained IBOutlet BITSDKColoredView *_feedbackComposeBackgroundView;
  __unsafe_unretained IBOutlet BITSDKColoredView *_horizontalLine;
  __unsafe_unretained IBOutlet BITSDKColoredView *_statusBar;
  __unsafe_unretained IBOutlet NSTextField *_contactInfoTextField;
  __unsafe_unretained IBOutlet NSTextField *_userNameTextField;
  __unsafe_unretained IBOutlet NSTextField *_userEmailTextField;
  __unsafe_unretained IBOutlet NSTextField *_statusBarTextField;
  __unsafe_unretained IBOutlet NSButton *_userDataContinueButton;
  __unsafe_unretained IBOutlet NSButton *_statusBarRefreshButton;
  __unsafe_unretained IBOutlet NSButton *_sendMessageButton;
  NSString *_userName, *_userEmail;
  __unsafe_unretained IBOutlet NSView *_feedbackView;
  __unsafe_unretained IBOutlet NSView *_feedbackEmptyView;
  __unsafe_unretained IBOutlet NSView *_statusBarComposeView;
  __unsafe_unretained IBOutlet NSView *_statusBarDefaultView;
  __unsafe_unretained IBOutlet NSImageView *_feedbackEmptyAppImageView;
  __unsafe_unretained IBOutlet NSScrollView *_feedbackComposeScrollView;
  __unsafe_unretained IBOutlet NSScrollView *_feedbackAttachmentsScrollView;
  __unsafe_unretained IBOutlet NSScrollView *_feedbackScrollView;
  __unsafe_unretained IBOutlet NSTableView *_feedbackAttachmentsTableView;
  __unsafe_unretained IBOutlet NSTableView *_feedbackTableView;
  __unsafe_unretained IBOutlet BITSDKTextView *_messageTextField;
  NSAttributedString *_messageText;
  __unsafe_unretained IBOutlet NSProgressIndicator *_statusBarLoadingIndicator;
  NSMutableArray *_attachments;
  NSOperationQueue *_thumbnailQueue;
  QLPreviewPanel *_previewPanel;
  BITFeedbackMessageAttachment *_previewAttachment;
  NSRect _previewThumbnailRect;
  __unsafe_unretained IBOutlet NSArrayController *_composeAttacchmentsArrayController;
}

- (id)initWithManager:(BITFeedbackManager *)feedbackManager;

@end
