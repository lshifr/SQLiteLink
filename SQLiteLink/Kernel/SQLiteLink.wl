Package["SQLiteLink`"]


PackageExport["SQLiteConnection"]
PackageExport["SQLiteCreateConnection"]
PackageExport["SQLiteDestroyConnection"]
PackageExport["SQLiteConnect"]
PackageExport["SQLiteDisconnect"]
PackageExport["SQLiteConnectedQ"]
PackageExport["SQLiteSQLExecute"]


Scan[
    (Unprotect[#]; ClearAll[#]) &, 
    Map["SQLiteLink`"<>#&, {"*", "*`*", "*`*`*"}]
]


$SQLiteLinkLibResourceDir = FileNameJoin[{
    PacletManager`PacletResource["SQLiteLink", "LibraryResources"],
    $SystemID
}]

$SQLiteLinkLib = FileNameJoin[{
    $SQLiteLinkLibResourceDir,
    "sqlitelink" <> "." <> Lookup[
        <|"MacOSX" -> "dylib", "Windows" -> "dll", "Unix" -> "so"|>, 
        $OperatingSystem
    ]
}]


newConnection       =   LibraryFunctionLoad[
    $SQLiteLinkLib, "SQLiteLink_new_connection", {"UTF8String"}, _Integer
]

destroyConnection   =   LibraryFunctionLoad[
    $SQLiteLinkLib, "SQLiteLink_destroy_connection", {_Integer}, _Integer
]

connect             =   LibraryFunctionLoad[
    $SQLiteLinkLib, "SQLiteLink_connect", {_Integer}, _Integer
]

disconnect          =   LibraryFunctionLoad[
    $SQLiteLinkLib, "SQLiteLink_disconnect", {_Integer}, _Integer
]

isConnected         =   LibraryFunctionLoad[
    $SQLiteLinkLib, "SQLiteLink_is_connected", {_Integer}, _Integer
]

execute             =   LibraryFunctionLoad[
    $SQLiteLinkLib, "SQLiteLink_execute", {_Integer, "UTF8String"}, _Integer
]

getSerielizedString =   LibraryFunctionLoad[
    $SQLiteLinkLib, "SQLiteLink_get_serialized_string", {_Integer}, "UTF8String"
]

getErrorString      =   LibraryFunctionLoad[
    $SQLiteLinkLib, "SQLiteLink_get_error_string", {_Integer}, "UTF8String"
]


SQLiteCreateConnection[path_String] := Replace[
    newConnection[path], 
    {
        index_Integer?NonNegative :> SQLiteConnection[index],
        errorCode_ :> Failure[
            "SQLiteLink",  
            <|
                "MessageTemplate" -> "Failed to create new connection. Error code: `ErrorCode`",
                "MessageParameters" -> <|"ErrorCode" -> errorCode|>
            |>
        ]
    }
]


SQLiteDestroyConnection[SQLiteConnection[index_Integer?NonNegative]] := Replace[
    destroyConnection[index], 
    {
        0 :> Success["SQLiteLink", <| "MessageTemplate" -> "Connection destroyed successfully" |>]
        ,
        errorCode_ :> Failure[
            "SQLiteLink",  
            <|
                "MessageTemplate" -> "Failed to destroy the connection. Error code: `ErrorCode`",
                "MessageParameters" -> <|"ErrorCode" -> errorCode|>
            |>
        ]
    }
]


SQLiteConnect[SQLiteConnection[index_Integer?NonNegative]] := Replace[
    connect[index],
    {
        0 :> Success["SQLiteLink", <| "MessageTemplate" -> "Connected successfully" |>],
        1 :> Failure["SQLiteLink", <| "MessageTemplate" -> "Connection failed"|>],
        2 :> Failure["SQLiteLink", <| "MessageTemplate" -> "Invalid or non-existent connection"|>]
    }
]


SQLiteDisconnect[SQLiteConnection[index_Integer?NonNegative]] := Replace[
    disconnect[index],
    {
        0 :> Success["SQLiteLink", <| "MessageTemplate" -> "Disconnected successfully" |>],
        1 :> Failure["SQLiteLink", <| "MessageTemplate" -> "Failed to disconnect"|>],
        2 :> Failure["SQLiteLink", <| "MessageTemplate" -> "Invalid or non-existent connection"|>]
    }
]


SQLiteConnectedQ[SQLiteConnection[index_Integer?NonNegative]] := 
    Replace[
        isConnected[index], 
        { 1 -> True, 0 -> False }
    ]


$executeErrorMessages = 
    <|
        1 -> "Execution error encountered",
        2 -> "Invalid or non-existent connection",
        3 -> "Invalid or non-existent connection",
        4 -> "Connection disconnected"
    |>

SQLiteSQLExecute[SQLiteConnection[index_Integer?NonNegative], sql_String] :=
    Module[{errorCode = execute[index, sql], jsonString},
        If[errorCode =!= 0, 
            Return @ Failure[
                "SQLiteLink", 
                <| 
                    "MessageTemplate" -> Lookup[
                        $executeErrorMessages, errorCode, "Unrecognized internal error"
                    ]
                    ,
                    "SQLiteErrorMessage" -> Replace[
                        getErrorString[index],
                        _LibraryFunctionError :> "Not available"
                    ]
                |>
            ] 
        ];
        jsonString = Replace[
            getSerielizedString[index],
            _LibraryFunctionError :> Return @ Failure[
                "SQLiteLink", 
                <|"MessageTemplate" -> "Result retrieval failure"|>
            ]
        ];
        Replace[
            Quiet @ Developer`ReadRawJSONString[jsonString],
            $Failed :> Failure[
                "SQLiteLink", 
                <| "MessageTemplate" -> "Internal serialization error"|>
            ]
        ]
    ]
