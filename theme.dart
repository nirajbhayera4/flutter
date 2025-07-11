import 'package:flutter/material.dart';
import 'package:flutterprojects/utils/theme/custom_themes/appBarTheme.dart';
import 'package:flutterprojects/utils/theme/custom_themes/bottomTheme.dart';
import 'package:flutterprojects/utils/theme/custom_themes/checkboxTheme.dart';
import 'package:flutterprojects/utils/theme/custom_themes/outlineButtonTheme.dart';
import 'package:flutterprojects/utils/theme/custom_themes/text_theme.dart';
import 'package:flutterprojects/utils/theme/custom_themes/elevatedbuttonTheme.dart';
import 'package:flutterprojects/utils/theme/custom_themes/textfieldTheme.dart';
class TAppTheme {
  TAppTheme._();
  static ThemeData lightTheme=ThemeData(
    useMaterial3: true,
    fontFamily: 'poppins',
    brightness: Brightness.light,
    primaryColor: Colors.blue,
    scaffoldBackgroundColor: Colors.white,
    textTheme: TTextTheme.lightTextTheme,
    elevatedButtonTheme: TElevatedButtonTheme.lightElevatedButtonTheme,
    appBarTheme: TAppBarTheme.lightAppBarTheme,
    checkboxTheme: TCheckboxTheme.lightcheckTheme,
    bottomSheetTheme: TBottomTheme.lightBottomTheme,
    outlinedButtonTheme: TOutlinedButtonTheme.lightoutlineButtonTheme,
    inputDecorationTheme: TTextfieldTheme.lightTextFieldTheme,

  );
  static ThemeData darkTheme=ThemeData(
    useMaterial3: true,
    fontFamily: 'poppins',
    brightness: Brightness.light,
    primaryColor: Colors.blue,
    scaffoldBackgroundColor: Colors.white,
    textTheme: TTextTheme.darkTextTheme,
    elevatedButtonTheme: TElevatedButtonTheme.darkElevatedButtonTheme,
    appBarTheme: TAppBarTheme.darkAppBarTheme,
    checkboxTheme: TCheckboxTheme.darkcheckTheme,
    bottomSheetTheme: TBottomTheme.darkBottomTheme,
    outlinedButtonTheme: TOutlinedButtonTheme.darkoutlineButtonTheme,
    inputDecorationTheme: TTextfieldTheme.darkTextFieldTheme,

  );



}