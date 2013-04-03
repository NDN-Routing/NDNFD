# Face::type()

Face::type() is a runtime typeid that identifies the type of face.

Each subclass of Face must have its unique FaceType.

## Defined FaceType ranges

* DgramFace subclasses: 100-199  
  DgramFace::IsDgramFaceType tests for this range
* StreamFace subclasses: 200-299  
  StreamFace::IsStreamFaceType tests for this range
* StreamListener subclasses: 300-399
  StreamListener::IsStreamListenerFaceType tests for this range

## Defined FaceTypes

* InternalClientFace: 10
* DgramFace: 100  
* DgramFallbackFace: 101
* StreamFace: 200  
* StreamListener: 300  
* ns-3 AppFace: 901

