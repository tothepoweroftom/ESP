#include "ostream.h"
#include "ofUtils.h"

#ifdef TARGET_WIN32
#define _WINSOCKAPI_
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#endif

#include <thread>
#include "ofxTCPClient.h"

#if __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

void MacOSKeyboardOStream::sendKey(char c) {
#if __APPLE__
    if (ofGetElapsedTimeMillis() < elapsed_time_ + kGracePeriod) {
        return;
    }
    elapsed_time_ = ofGetElapsedTimeMillis();

    // Get the process number for the front application.
    ProcessSerialNumber psn = { 0, kNoProcess };
    GetFrontProcess( &psn );

    UniChar uni_char = c;
    CGEventRef key_down = CGEventCreateKeyboardEvent(NULL, 0, true);
    CGEventRef key_up = CGEventCreateKeyboardEvent(NULL, 0, false);
    CGEventKeyboardSetUnicodeString(key_down, 1, &uni_char);
    CGEventKeyboardSetUnicodeString(key_up, 1, &uni_char);
    CGEventPostToPSN(&psn, key_down);
    CGEventPostToPSN(&psn, key_up);
    CFRelease(key_down);
    CFRelease(key_up);
#endif
}

void MacOSKeyboardOStream::sendKeyCode(uint16_t key) {
#if __APPLE__
    if (ofGetElapsedTimeMillis() < elapsed_time_ + kGracePeriod) {
        return;
    }
    elapsed_time_ = ofGetElapsedTimeMillis();

    // Get the process number for the front application.
    ProcessSerialNumber psn = { 0, kNoProcess };
    GetFrontProcess( &psn );

    CGEventRef key_down = CGEventCreateKeyboardEvent(NULL, key, true);
    CGEventRef key_up = CGEventCreateKeyboardEvent(NULL, key, false);
    CGEventPostToPSN(&psn, key_down);
    CGEventPostToPSN(&psn, key_up);
    CFRelease(key_down);
    CFRelease(key_up);
#endif
}

void MacOSKeyboardOStream::sendString(const std::string& str) {
#if __APPLE__
    // Get the process number for the front application.
    ProcessSerialNumber psn = { 0, kNoProcess };
    GetFrontProcess( &psn );

    UniChar s[str.length()];
    for (uint32_t i = 0; i < str.length(); i++) {
        s[i] = str[i];
    }

    CGEventRef e = CGEventCreateKeyboardEvent(NULL, 0, true);
    CGEventKeyboardSetUnicodeString(e, str.length(), s);
    CGEventPostToPSN(&psn, e);
    CFRelease(e);
#endif
}

void MacOSMouseOStream::clickMouse(pair<uint32_t, uint32_t> mouse) {
#if __APPLE__
    if (ofGetElapsedTimeMillis() < elapsed_time_ + kGracePeriod) {
        return;
    }
    elapsed_time_ = ofGetElapsedTimeMillis();

    doubleClick(mouse);
#endif
}

void MacOSMouseOStream::doubleClick(pair<uint32_t, uint32_t> mouse, int clickCount) {
#if __APPLE__
    CGPoint point = CGPointMake(mouse.first, mouse.second);
    CGEventRef theEvent = CGEventCreateMouseEvent(
        NULL, kCGEventLeftMouseDown, point, kCGMouseButtonLeft);

    ProcessSerialNumber psn = { 0, kNoProcess };
    GetFrontProcess( &psn );

    CGEventSetIntegerValueField(theEvent, kCGMouseEventClickState, clickCount);
    CGEventPostToPSN(&psn, theEvent);
    CGEventSetType(theEvent, kCGEventLeftMouseUp);
    CGEventPostToPSN(&psn, theEvent);
    CGEventSetType(theEvent, kCGEventLeftMouseDown);
    CGEventPostToPSN(&psn, theEvent);
    CGEventSetType(theEvent, kCGEventLeftMouseUp);
    CGEventPostToPSN(&psn, theEvent);
    CFRelease(theEvent);
#endif
}

bool TcpOStream::start() {
    if (client_ == nullptr) {
        client_ = new ofxTCPClient();
    }
    has_started_ = client_->setup(server_, port_);
    return has_started_;
}

void TcpOStream::sendString(const string& tosend) {
    if (client_ != nullptr && client_->isConnected()) {
        client_->sendRaw(tosend);
    }

    // If not connected, we start a new thread that tries to connect
    if (!client_->isConnected() && !is_in_retry_) {
        is_in_retry_ = true;
        auto t = std::thread([this] () {
            while (!this->client_->setup(server_, port_)) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            is_in_retry_ = false;
        });
        t.detach();
    }
}
