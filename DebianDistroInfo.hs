{- Copyright (C) 2011, Benjamin Drung <bdrung@debian.org>

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
-}

module DebianDistroInfo where

import Data.List
import Data.Time
import System.Console.GetOpt
import System.IO
import System.Locale
import System

import Text.CSV

import DistroInfo

-- | Trim whitespaces from given string (similar to Python function)
trim :: String -> String
trim = reverse . trimLeft . reverse . trimLeft
  where trimLeft = dropWhile (`elem` " \t\n\r")

data Options = Options { optDate :: Day
                       , optFilter :: Maybe (Day -> [DebianEntry] -> [DebianEntry])
                       , optFormat :: DebianEntry -> String
                       }

startOptions :: IO Options
startOptions = do
  today <- getCurrentTime
  return Options { optDate = utctDay today,
                   optFilter  = Nothing,
                   optFormat  = debSeries
                 }

options :: [OptDescr (Options -> IO Options)]
options =
  let
    debVersionOrSeries e =
      let
        version = debVersion e
      in
        if version /= ""
        then version
        else debSeries e
    readDate arg opt =
      return opt { optDate = readTime defaultTimeLocale "%F" arg }
    printHelp _ =
      do
        prg <- getProgName
        hPutStr stderr ("Usage: " ++ prg ++ " [options]\n\nOptions:")
        hPutStrLn stderr (usageInfo "" options)
        hPutStrLn stderr ("See " ++ prg ++ "(1) for more info.")
        exitWith ExitSuccess
  in
    [ Option "h" ["help"] (NoArg printHelp) "show this help message and exit"
    , Option "" ["date"] (ReqArg readDate "DATE")
             "date for calculating the version (default: today)"
    , Option "a" ["all"]
             (NoArg (\ opt -> return opt { optFilter = Just debianAll }))
             "list all known versions"
    , Option "d" ["devel"]
             (NoArg (\ opt -> return opt { optFilter = Just debianDevel }))
             "latest development version"
    , Option "o" ["oldstable"]
             (NoArg (\ opt -> return opt { optFilter = Just debianOldstable }))
             "latest oldstable version"
    , Option "s" ["stable"]
             (NoArg (\ opt -> return opt { optFilter = Just debianStable }))
             "latest stable version"
    , Option "" ["supported"]
             (NoArg (\opt -> return opt { optFilter = Just debianSupported }))
             "list of all supported stable versions"
    , Option "t" ["testing"]
             (NoArg (\ opt -> return opt { optFilter = Just debianTesting }))
             "current testing version"
    , Option "" ["unsupported"]
             (NoArg (\opt -> return opt { optFilter = Just debianUnsupported }))
             "list of all unsupported stable versions"
    , Option "c" ["codename"]
             (NoArg (\ opt -> return opt { optFormat = debSeries }))
             "print the codename"
    , Option "r" ["release"]
             (NoArg (\ opt -> return opt { optFormat = debVersionOrSeries }))
             "print the release version"
    , Option "f" ["fullname"]
             (NoArg (\ opt -> return opt { optFormat = debFull }))
             "print the full name"
    ]

main :: IO ()
main = do
  args <- getArgs
  case getOpt RequireOrder options args of
    (actions, [], []) -> do
      Options { optDate    = date, optFilter  = maybeFilter,
                optFormat  = format } <- foldl (>>=) startOptions actions
      maybeCsv <- parseCSVFromFile "/usr/share/distro-info/debian.csv"
      case maybeFilter of
        Nothing -> error ("You have to select exactly one of " ++
                          "--all, --devel, --oldstable, --stable, " ++
                          "--supported, --testing, --unsupported.")
        Just debianFilter ->
          case maybeCsv of
            Left errorMsg -> print errorMsg
            Right csvData ->
              let
                result = map format $ debianFilter date $ debianEntry csvData
              in
                putStrLn $ concat $ intersperse "\n" result
    (_, nonOptions, []) ->
      error $ "unrecognized arguments: " ++ unwords nonOptions
    (_, _, msgs) -> error $ trim $ concat msgs
