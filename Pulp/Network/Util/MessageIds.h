#pragma once


X_NAMESPACE_BEGIN(net)

X_DECLARE_ENUM8(MessageID)(

	// the remote systems protcol version is incompatible
	/// action: read remote clients protcol version display in error msg.
	ProtcolVersionIncompatible,

	// ----- Connection related ----- 

	// a Ping from a connected system
	/// action: update timestamps
	ConnectedPing,
	// a Pong from a connected system
	/// action: update timestamps
	ConnectedPong,
	// a Ping from a un-connected system
	/// action: reply, skip timestamp update.
	UnConnectedPing,
	// a Pong from a un-connected system
	/// action: reply, skip timestamp update.
	UnConnectedPong,
	// request to connect.
	/// action: 
	OpenConnectionRequest,
	// a connection response.
	/// action: 
	OpenConnectionResponse,
	// connection to server accepted.
	/// action: 
	ConnectionRequestAccepted,
	// connection to server failed. (reasons this can happen: ..?)
	/// action: 
	ConnectionRequestFailed,

	// you are already connected to this remote system
	/// action: 
	AlreadyConnected,

	// connection to remote system has been lost
	/// action: 
	ConnectionLost,
	// banned from connecting to target remove system
	/// action: cry.
	ConnectionBanned,
	// the remote system has not accepting any more connections at this time.
	/// action: give up or try again later
	ConnectionNoFreeNoSlots,
	// the remote system has rejected the connection for a given amount of time.
	/// action: read the ratelimit time, and try again after elapsed.
	ConnectionRateLimited,

	// ----- Misc ----- 

	// a timestamp from remote system.
	/// action: 
	TimeStamp,
	// send test on socket.
	/// action: ignore
	SendTest,


	// ----- Nat ----- 

	NatPunchthroughRequest,
	NatPunchthroughFailed,
	NatPunchthroughSucceeded,
	NatTypeDetectionRequest,
	NatTypeDetectionResult

);

X_NAMESPACE_END
