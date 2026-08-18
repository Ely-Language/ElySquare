// error codes
// ===============

// different error codes for ElySquare
enum class ErrorCode {
    Success=0,               // 0 - no errors
    OutOfMemory=1,           // 1 - virtual memory pool out of space
    DanglingPointerAccess=2, // 2 - accessing a pointer that has been freed
    TypeNotRegistered=3,     // 3 - serialized/unpacked type not registered
    MailboxKeyNotFound=4,    // 4 - mailboxes key not found
    SafepointTimeout=5,      // 5 - thread didn't recognized GC stop command
    ContainerContainsOther=6 // 6 - this container has no thing with that property
};