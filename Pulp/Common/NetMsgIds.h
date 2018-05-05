#pragma once

X_NAMESPACE_BEGIN(net)

//
//	Connection process:
//
//		send -> OpenConnectionRequest
//				protoColVersion
//
//		recive <- OpenConnectionResponse
//				remoteGuid
//				mtu
//
//		send -> OpenConnectionRequestStage2
//				clientGuid
//				bidingAdd
//				mtu
//
//				we try to add the client as a remote system here.
//				remoteMode -> UnverifiedSender
//
//		recive <- OpenConnectionResponseStage2
//				remoteGuid
//				bindingAdd
//				mtu
//
//				we try to add the client as a remote (marking it as a conection we opened)
//				remoteMode -> UnverifiedSender
//
//		send -> ConnectionRequest
//				guid
//				timeStamp
//				password?
//				remoteMode -> RequestedConnection
//
//		recive <- ConnectionRequestAccepted
//				systemAddres server see's for us
//				systemIndex
//				listofLocalBindings
//				ConnectionRequest::timeStamp
//				serverTimeStamp
//				remoteMode -> HandlingConnectionRequest
//
//		send -> ConnectionRequestHandshake
//				systemAddres of server from our point of view
//				listOfLocalBindings
//				ConnectionRequestAccepted::serverTimeStamp
//				timeStamp
//				remoteMode -> Connected
//
//		Server recives -> ConnectionRequestHandshake
//				updates remote info
//				remoteMode -> Connected
//
//		connection is complete.
//		Ping pong commences
//
//
//	Messages accepted for various remote modes:
//
//		UnverifiedSender:
//			ConnectionRequest
//
//		RequestedConnection:
//			ConnectionRequestAccepted
//
//		HandlingConnectionRequest:
//			ConnectionRequestHandshake
//
//		Connected:
//
//
//
//
//
//

X_DECLARE_ENUM8(MessageID)
(

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
    // a Ping from a un-connected system
    /// action: reply if we have open connections, skip timestamp update.
    UnConnectedPingOpenConnections,
    // a Pong from a un-connected system
    /// action: reply, skip timestamp update.
    UnConnectedPong,
    // request to OPEN connection.
    /// action: respond with OpenConnectionResponse
    OpenConnectionRequest,
    // request to OPEN connection stage2.
    /// action:  respond with OpenConnectionResponseStage2
    OpenConnectionRequestStage2,
    // a connection OPEN response.
    /// action: send a OpenConnectionRequestStage2
    OpenConnectionResponse,
    // a connection OPEN response stage 2.
    /// action: send ConnectionRequest
    OpenConnectionResponseStage2,

    // send after we have performed openConnection stages 1 && 2
    // we can send this to finaly open the connection.
    /// action: respond with ConnectionRequestAccepted || ConnectionRequestFailed
    ConnectionRequest,
    // connection to server accepted.
    /// action: respond with ConnectionRequestHandShake
    ConnectionRequestAccepted,
    // client responded to connection accepted
    /// action: mark client as connected
    ConnectionRequestHandShake,
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
    ConnectionNoFreeSlots,
    // the remote system has rejected the connection for a given amount of time.
    /// action: read the ratelimit time, and try again after elapsed.
    ConnectionRateLimited,

    // the remote system has disconnected from us. if we client server has closed. if server, client has left.
    /// action: panic!
    DisconnectNotification,

    InvalidPassword,

    // ----------------

    SndReceiptAcked,
    SndReceiptLost,

    // ----- Misc -----

    // a remote system reposts stu as still missing
    /// action: keep searching.
    StuNotFnd,

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
    NatTypeDetectionResult,

    // --- Chat ---

    ChatMsg,

    // --- Session ---

    LoadingStart,
    LoadingDone,
    InGame,

    SnapShotAck
);

X_NAMESPACE_END
