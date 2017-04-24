#pragma once

#pragma pack(1)

typedef struct _BACKUP_MESSAGE
{
    FILTER_MESSAGE_HEADER MessageHeader; // Required structure header
    BACKUP_NOTIFICATION Notification; // Private backup-specific fields begin here
    OVERLAPPED Ovlp; // Overlapped structure: this is not really part of the message However we embed it instead of using a separately allocated overlap structure
} BACKUP_MESSAGE, *PBACKUP_MESSAGE;

typedef struct _BACKUP_REPLY_MESSAGE
{
    FILTER_REPLY_HEADER ReplyHeader; // Required structure header
    BACKUP_REPLY Reply; // Private scanner-specific fields begin here
} BACKUP_REPLY_MESSAGE, *PBACKUP_REPLY_MESSAGE;
