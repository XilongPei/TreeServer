/* By Niu Jingyu 1998.03 */

PSID GetUserSid ( void );
BOOL GetAccountSid ( LPTSTR SystemName, LPTSTR AccountName, PSID *Sid );
PSID CreateWorldSid ( void );
PSID CreateSystemSid ( void );
PSID CreateAnonymousSid ( void );
PSID CreateInteractiveSid ( void );
