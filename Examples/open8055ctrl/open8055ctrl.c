/* ------------------------------------------------------------
 * open8055ctrl.c
 *
 *  A generic program to test the libopen8055.dll.
 * ------------------------------------------------------------
 */
#include "open8055.h"
#include "open8055ctrl.h"
#include "gram.h"


/* ----
 * Global data
 * ----
 */
int			terminateFlag = FALSE;
int			commandTag;
OPEN8055_HANDLE		cardHandle;

/* ----
 * Local definitions
 * ----
 */
#define     CMDLINE_SIZE            1024

typedef struct {
    int		commandTag;
    char	*usageText;
} usage_help_t;


/* ----
 * Local data
 * ----
 */
static char                    *boardDestination = "card0";
static char			commandLine[CMDLINE_SIZE];

static usage_help_t		usageHelp[] = {
    { C_LastError,		"LastError" },
    { C_CardPresent,		"CardPresent <cardNum>" },
    { C_Close,			"Close" },
    { C_Reset,			"Reset" },

    { C_Wait,			"Wait <milliseconds>" },
    { C_WaitFor,		"WaitFor <inputMask> <milliseconds>" },
    { C_GetAutoFlush,		"GetAutoFlush" },
    { C_SetAutoFlush,		"SetAutoFlush <flag>" },
    { C_Flush,			"Flush" },

    { C_GetInputDigital,	"GetInputDigital <channel>" },
    { C_GetInputDigitalAll,	"GetInputDigitalAll" },
    { C_GetInputCounter,	"GetInputCounter <channel>" },
    { C_ResetInputCounter,	"ResetInputCounter <channel>" },
    { C_ResetInputCounterAll,	"ResetInputCounterAll" },
    { C_GetInputDebounce,	"GetInputDebounce <channel>" },
    { C_SetInputDebounce,	"SetInputDebounce <channel> <milliseconds>" },
    { C_GetInputADC,		"GetInputADC <channel>" },

    { C_GetOutputDigital,	"SetOutputDigital <channel>" },
    { C_GetOutputDigitalAll,	"SetOutputDigitalAll" },
    { C_GetOutputPWM,		"SetOutputPWM <channel>" },
    { C_SetOutputDigital,	"SetOutputDigital <channel> <flag>" },
    { C_SetOutputDigitalAll,	"SetOutputDigitalAll <bits>" },
    { C_SetOutputPWM,		"SetOutputPWM <channel> <dutyCycle>" },
    { 0, NULL }
};



/* ----
 * Local functions
 * ----
 */
static int	processCommand(char *cmd);

/* ----------
 * main()
 * ----------
 */
int
main(const int argc, char * const argv[])
{
    int		option_list = FALSE;
    int         errors = 0;
    int         c;
    int		rc = 0;

    /* ----
     * Parse command line options.
     * ----
     */
    while ((c = getopt(argc, argv, "d:hl")) >= 0)
    {
	switch (c)
	{
	    case 'd':   boardDestination = optarg;
			break;

	    case 'l':   option_list = 1;
			break;

	    case 'h':   errors++;
			break;

	    default:    fprintf(stderr, "ERROR unknown option -%c\n", c);
			errors++;
			break;
	}
    }

    /* ----
     * If we have an error so far, display usage message and exit.
     * ----
     */
    if (errors)
    {
	fprintf(stderr, "usage: open8055ctrl [options] [command]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "opitons:\n");
	fprintf(stderr, "   -d <destination>\n");
	fprintf(stderr, "   -l                     List available local boards\n");
	fprintf(stderr, "   -h                     Display this message\n");
	return 1;
    }

    /* ----
     * If -l is given, list the available boards and exit.
     * ----
     */
    if (option_list)
    {
    	int i;

	printf("Local Open8055 boards:\n");
	for (i = 0; i < 4; i++)
	{
	    if (Open8055_CardPresent(i))
	    {
		printf("  card%d\n", i);
	    }
	}
	return 0;
    }

    /* ----
     * Connect to the Open8055 board.
     * ----
     */
    if ((cardHandle = Open8055_Connect(boardDestination, NULL)) == NULL)
    {
	printf("ERROR Open8055_Connect(): %s\n", Open8055_LastError(NULL));
	return 2;
    }

    /* ----
     * Process command(s)
     * ----
     */
    if (optind < argc)
    {
	/* ----
	 * A command is given on the command line. Assemble and execute it.
	 * ----
	 */
	commandLine[0] = '\0';

	while (optind < argc && (strlen(commandLine) + strlen(argv[optind]) + 1) < CMDLINE_SIZE)
	{
	    strcat(commandLine, argv[optind++]);
	    strcat(commandLine, " ");
	}

	if (optind < argc)
	{
	    fprintf(stderr, "ERROR: command line too long\n");
	    return 1;
	}

	rc = processCommand(commandLine);
    }
    else
    {
	/* ----
	 * Interactive mode. Read and execute commands from stdin until EOF
	 * or something goes wrong with the card.
	 * ----
	 */
    	while (!terminateFlag && fgets(commandLine, sizeof(commandLine), stdin) != NULL)
	{
	    processCommand(commandLine);
	}
    }


    /* ----
     * Close the board.
     * ----
     */
    if (Open8055_Close(cardHandle) < 0)
    {
	printf("ERROR Open8055_Close(): %s\n", Open8055_LastError(NULL));
	return 3;
    }

    return rc;
}


static int
processCommand(char *cmd)
{
    int		rc;

    commandTag = T_UNKNOWN_CMD;
    scan_new_command(cmd);
    rc = yyparse();

    if (rc != 0)
    {
        if (commandTag == T_UNKNOWN_CMD)
	{
	    fprintf(stderr, "command not recognized: %s\n", cmd);
	    rc = 1;
	}
	else
	{
	    usage_help_t	*usage;

	    for (usage = usageHelp; usage->usageText != NULL; usage++)
	    {
	    	if (usage->commandTag == commandTag)
		    break;
	    }

	    if (usage->usageText != NULL)
	        printf("usage: %s\n", usage->usageText);
	    else
	        printf("internal error, command tag %d not found in usageHelp\n", commandTag);

	    rc = 1;
	}
    }

    return 0;
}


