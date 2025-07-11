import 'package:flutter/material.dart';
class TTextTheme{
  TTextTheme._();
  static TextTheme lightTextTheme =TextTheme(
    headlineLarge: const TextStyle().copyWith(fontSize: 32,fontWeight: FontWeight.bold,color: Colors.black),
    headlineMedium: const TextStyle().copyWith(fontSize: 24,fontWeight: FontWeight.bold,color: Colors.black),
    headlineSmall: const TextStyle().copyWith(fontSize: 18,fontWeight: FontWeight.bold,color: Colors.black),
    
    titleLarge: const TextStyle().copyWith(fontSize: 16,fontWeight: FontWeight.bold,color: Colors.black),
    titleMedium: const TextStyle().copyWith(fontSize: 16,fontWeight: FontWeight.bold,color: Colors.black),
    titleSmall: const TextStyle().copyWith(fontSize: 16,fontWeight: FontWeight.bold,color: Colors.black),
    
    bodyLarge:  const TextStyle().copyWith(fontSize: 14,fontWeight: FontWeight.bold,color: Colors.black),
    bodyMedium:  const TextStyle().copyWith(fontSize: 14,fontWeight: FontWeight.bold,color: Colors.black),
    bodySmall:  const TextStyle().copyWith(fontSize: 14,fontWeight: FontWeight.bold,color: Colors.black.withOpacity(0.5)),
    
    labelLarge:  const TextStyle().copyWith(fontSize: 12.0,fontWeight: FontWeight.bold,color: Colors.black),
    labelMedium:  const TextStyle().copyWith(fontSize: 12.0,fontWeight: FontWeight.bold,color: Colors.black.withOpacity(0.5)),
 
    
    
  );
  static TextTheme darkTextTheme =TextTheme(
    headlineLarge: const TextStyle().copyWith(fontSize: 32,fontWeight: FontWeight.bold,color: Colors.white),
    headlineMedium: const TextStyle().copyWith(fontSize: 24,fontWeight: FontWeight.bold,color: Colors.white),
    headlineSmall: const TextStyle().copyWith(fontSize: 18,fontWeight: FontWeight.bold,color: Colors.white),

    titleLarge: const TextStyle().copyWith(fontSize: 16,fontWeight: FontWeight.bold,color: Colors.white),
    titleMedium: const TextStyle().copyWith(fontSize: 16,fontWeight: FontWeight.bold,color: Colors.white),
    titleSmall: const TextStyle().copyWith(fontSize: 16,fontWeight: FontWeight.bold,color: Colors.white),

    bodyLarge:  const TextStyle().copyWith(fontSize: 14,fontWeight: FontWeight.bold,color: Colors.white),
    bodyMedium:  const TextStyle().copyWith(fontSize: 14,fontWeight: FontWeight.bold,color: Colors.white),
    bodySmall:  const TextStyle().copyWith(fontSize: 14,fontWeight: FontWeight.bold,color: Colors.white.withOpacity(0.5)),

    labelLarge:  const TextStyle().copyWith(fontSize: 12.0,fontWeight: FontWeight.bold,color: Colors.white),
    labelMedium:  const TextStyle().copyWith(fontSize: 12.0,fontWeight: FontWeight.bold,color: Colors.white.withOpacity(0.5)),
    
    
  );
}