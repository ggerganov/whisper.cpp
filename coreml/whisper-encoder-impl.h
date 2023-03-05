//
// CoremlEncoder.h
//
// This file was automatically generated and should not be edited.
//

#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#include <stdint.h>
#include <os/log.h>

NS_ASSUME_NONNULL_BEGIN


/// Model Prediction Input Type
API_AVAILABLE(macos(10.15), ios(13.0), watchos(6.0), tvos(13.0)) __attribute__((visibility("hidden")))
@interface CoremlEncoderInput : NSObject<MLFeatureProvider>

/// melSegment as 1 × 80 × 3000 3-dimensional array of floats
@property (readwrite, nonatomic, strong) MLMultiArray * melSegment;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithMelSegment:(MLMultiArray *)melSegment NS_DESIGNATED_INITIALIZER;

@end


/// Model Prediction Output Type
API_AVAILABLE(macos(10.15), ios(13.0), watchos(6.0), tvos(13.0)) __attribute__((visibility("hidden")))
@interface CoremlEncoderOutput : NSObject<MLFeatureProvider>

/// output as multidimensional array of floats
@property (readwrite, nonatomic, strong) MLMultiArray * output;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithOutput:(MLMultiArray *)output NS_DESIGNATED_INITIALIZER;

@end


/// Class for model loading and prediction
API_AVAILABLE(macos(10.15), ios(13.0), watchos(6.0), tvos(13.0)) __attribute__((visibility("hidden")))
@interface CoremlEncoder : NSObject
@property (readonly, nonatomic, nullable) MLModel * model;

/**
    URL of the underlying .mlmodelc directory.
*/
+ (nullable NSURL *)URLOfModelInThisBundle;

/**
    Initialize CoremlEncoder instance from an existing MLModel object.

    Usually the application does not use this initializer unless it makes a subclass of CoremlEncoder.
    Such application may want to use `-[MLModel initWithContentsOfURL:configuration:error:]` and `+URLOfModelInThisBundle` to create a MLModel object to pass-in.
*/
- (instancetype)initWithMLModel:(MLModel *)model NS_DESIGNATED_INITIALIZER;

/**
    Initialize CoremlEncoder instance with the model in this bundle.
*/
- (nullable instancetype)init;

/**
    Initialize CoremlEncoder instance with the model in this bundle.

    @param configuration The model configuration object
    @param error If an error occurs, upon return contains an NSError object that describes the problem. If you are not interested in possible errors, pass in NULL.
*/
- (nullable instancetype)initWithConfiguration:(MLModelConfiguration *)configuration error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/**
    Initialize CoremlEncoder instance from the model URL.

    @param modelURL URL to the .mlmodelc directory for CoremlEncoder.
    @param error If an error occurs, upon return contains an NSError object that describes the problem. If you are not interested in possible errors, pass in NULL.
*/
- (nullable instancetype)initWithContentsOfURL:(NSURL *)modelURL error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/**
    Initialize CoremlEncoder instance from the model URL.

    @param modelURL URL to the .mlmodelc directory for CoremlEncoder.
    @param configuration The model configuration object
    @param error If an error occurs, upon return contains an NSError object that describes the problem. If you are not interested in possible errors, pass in NULL.
*/
- (nullable instancetype)initWithContentsOfURL:(NSURL *)modelURL configuration:(MLModelConfiguration *)configuration error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/**
    Construct CoremlEncoder instance asynchronously with configuration.
    Model loading may take time when the model content is not immediately available (e.g. encrypted model). Use this factory method especially when the caller is on the main thread.

    @param configuration The model configuration
    @param handler When the model load completes successfully or unsuccessfully, the completion handler is invoked with a valid CoremlEncoder instance or NSError object.
*/
+ (void)loadWithConfiguration:(MLModelConfiguration *)configuration completionHandler:(void (^)(CoremlEncoder * _Nullable model, NSError * _Nullable error))handler API_AVAILABLE(macos(11.0), ios(14.0), watchos(7.0), tvos(14.0)) __attribute__((visibility("hidden")));

/**
    Construct CoremlEncoder instance asynchronously with URL of .mlmodelc directory and optional configuration.

    Model loading may take time when the model content is not immediately available (e.g. encrypted model). Use this factory method especially when the caller is on the main thread.

    @param modelURL The model URL.
    @param configuration The model configuration
    @param handler When the model load completes successfully or unsuccessfully, the completion handler is invoked with a valid CoremlEncoder instance or NSError object.
*/
+ (void)loadContentsOfURL:(NSURL *)modelURL configuration:(MLModelConfiguration *)configuration completionHandler:(void (^)(CoremlEncoder * _Nullable model, NSError * _Nullable error))handler API_AVAILABLE(macos(11.0), ios(14.0), watchos(7.0), tvos(14.0)) __attribute__((visibility("hidden")));

/**
    Make a prediction using the standard interface
    @param input an instance of CoremlEncoderInput to predict from
    @param error If an error occurs, upon return contains an NSError object that describes the problem. If you are not interested in possible errors, pass in NULL.
    @return the prediction as CoremlEncoderOutput
*/
- (nullable CoremlEncoderOutput *)predictionFromFeatures:(CoremlEncoderInput *)input error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/**
    Make a prediction using the standard interface
    @param input an instance of CoremlEncoderInput to predict from
    @param options prediction options
    @param error If an error occurs, upon return contains an NSError object that describes the problem. If you are not interested in possible errors, pass in NULL.
    @return the prediction as CoremlEncoderOutput
*/
- (nullable CoremlEncoderOutput *)predictionFromFeatures:(CoremlEncoderInput *)input options:(MLPredictionOptions *)options error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/**
    Make a prediction using the convenience interface
    @param melSegment as 1 × 80 × 3000 3-dimensional array of floats:
    @param error If an error occurs, upon return contains an NSError object that describes the problem. If you are not interested in possible errors, pass in NULL.
    @return the prediction as CoremlEncoderOutput
*/
- (nullable CoremlEncoderOutput *)predictionFromMelSegment:(MLMultiArray *)melSegment error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/**
    Batch prediction
    @param inputArray array of CoremlEncoderInput instances to obtain predictions from
    @param options prediction options
    @param error If an error occurs, upon return contains an NSError object that describes the problem. If you are not interested in possible errors, pass in NULL.
    @return the predictions as NSArray<CoremlEncoderOutput *>
*/
- (nullable NSArray<CoremlEncoderOutput *> *)predictionsFromInputs:(NSArray<CoremlEncoderInput*> *)inputArray options:(MLPredictionOptions *)options error:(NSError * _Nullable __autoreleasing * _Nullable)error;
@end

NS_ASSUME_NONNULL_END
