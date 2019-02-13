////////////////////////////////////////////////////////////////////
//
// NetData.dll & SysLogClient.dll

// Define SysLogClient Dll
#define		ID_SYSLOG_SEVERITY_EMERGENCY		0		// Emergency: system is unusable
#define		ID_SYSLOG_SEVERITY_ALERT			1		// Alert: action must be taken immediately
#define		ID_SYSLOG_SEVERITY_CRITICAL			2		// Critical: critical conditions
#define		ID_SYSLOG_SEVERITY_ERROR			3		// Error: error conditions
#define		ID_SYSLOG_SEVERITY_WARNING			4		// Warning: warning conditions
#define		ID_SYSLOG_SEVERITY_NOTICE			5		// Notice: normal but significant condition
#define		ID_SYSLOG_SEVERITY_INFORMATIONAL	6		// Informational: informational messages
#define		ID_SYSLOG_SEVERITY_DEBUG			7		// Debug: debug-level messages

#define		SYSLOGCLIENTDLL		"SysLogClient.dll"
typedef char* (__cdecl *SYSLOGSEND) (char* aMsg, WORD ASeverity);

// Load SysLogClient Dll
extern SYSLOGSEND			SysLogSend;
extern WORD					wSeverity;
extern CRITICAL_SECTION		csSysLog;

BOOL						LoadSysLogClientDll(void);
void						UnLoadSysLogClientDll(void);
char*						LogSend(char* aMsg, WORD ASeverity);
void						SetServerity(WORD ASeverity,BOOL bEnable);
BOOL						GetServerity(WORD ASeverity);