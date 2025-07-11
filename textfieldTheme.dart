import 'package:flutter/material.dart';
class TTextfieldTheme {
  TTextfieldTheme._();
  static InputDecorationTheme lightTextFieldTheme=InputDecorationTheme(
    errorMaxLines: 3,
    prefixIconColor: Colors.grey,
    suffixIconColor: Colors.grey,
    labelStyle: const TextStyle().copyWith(fontSize: 14,color: Colors.black),
    hintStyle: const TextStyle().copyWith(fontSize: 14,color: Colors.black),
    errorStyle: const TextStyle().copyWith(fontStyle: FontStyle.normal),
    floatingLabelStyle: const TextStyle().copyWith(color: Colors.black.withValues(alpha: 0.8)),
    border: const OutlineInputBorder().copyWith(borderRadius: BorderRadius.circular(14),
    borderSide: const BorderSide(width: 1,color: Colors.grey),
    ),
    enabledBorder: const OutlineInputBorder().copyWith(
      borderRadius:  BorderRadius.circular(14),
      borderSide: const BorderSide(width: 1,color: Colors.grey),
    ),
    focusedBorder: const OutlineInputBorder().copyWith(
      borderRadius: BorderRadius.circular(14),
      borderSide: const BorderSide(width: 1,color: Colors.black)
    ),
    errorBorder: const OutlineInputBorder().copyWith(
      borderSide: const BorderSide(width: 1,color: Colors.red),
      borderRadius: BorderRadius.circular(14),
    ),
    focusedErrorBorder: const OutlineInputBorder().copyWith(
      borderRadius: BorderRadius.circular(14),
      borderSide: const BorderSide(width: 1,color: Colors.orange),
    ),
  );
  static InputDecorationTheme darkTextFieldTheme=InputDecorationTheme(

    errorMaxLines: 3,
    prefixIconColor: Colors.grey,
    suffixIconColor: Colors.grey,
    labelStyle: const TextStyle().copyWith(fontSize: 14,color: Colors.black),
    hintStyle: const TextStyle().copyWith(fontSize: 14,color: Colors.black),
    errorStyle: const TextStyle().copyWith(fontStyle: FontStyle.normal),
    floatingLabelStyle: const TextStyle().copyWith(color: Colors.black.withValues(alpha: 0.8)),
    border: const OutlineInputBorder().copyWith(borderRadius: BorderRadius.circular(14),
      borderSide: const BorderSide(width: 1,color: Colors.grey),
    ),
    enabledBorder: const OutlineInputBorder().copyWith(
      borderRadius:  BorderRadius.circular(14),
      borderSide: const BorderSide(width: 1,color: Colors.grey),
    ),
    focusedBorder: const OutlineInputBorder().copyWith(
        borderRadius: BorderRadius.circular(14),
        borderSide: const BorderSide(width: 1,color: Colors.black)
    ),
    errorBorder: const OutlineInputBorder().copyWith(
      borderSide: const BorderSide(width: 1,color: Colors.red),
      borderRadius: BorderRadius.circular(14),
    ),
    focusedErrorBorder: const OutlineInputBorder().copyWith(
      borderRadius: BorderRadius.circular(14),
      borderSide: const BorderSide(width: 1,color: Colors.orange),
    ),
  );
}