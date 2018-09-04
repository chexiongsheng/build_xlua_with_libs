﻿using System;
using System.Linq;

namespace XLua
{

    internal static class TypeExtensions
    {
        public static bool IsValueType(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsValueType;
#else
            return type.GetTypeInfo().IsValueType;
#endif
        }

        public static bool IsEnum(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsEnum;
#else
            return type.GetTypeInfo().IsEnum;
#endif
        }

        public static bool IsPrimitive(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsPrimitive;
#else
            return type.GetTypeInfo().IsPrimitive;
#endif
        }

        public static bool IsAbstract(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsAbstract;
#else
            return type.GetTypeInfo().IsAbstract;
#endif
        }

        public static bool IsSealed(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsSealed;
#else
            return type.GetTypeInfo().IsSealed;
#endif
        }

        public static bool IsInterface(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsInterface;
#else
            return type.GetTypeInfo().IsInterface;
#endif
        }

        public static bool IsClass(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsClass;
#else
            return type.GetTypeInfo().IsClass;
#endif
        }

        public static Type BaseType(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.BaseType;
#else
            return type.GetTypeInfo().BaseType;
#endif
        }

        public static bool IsGenericType(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsGenericType;
#else
            return type.GetTypeInfo().IsGenericType;
#endif
        }

        public static bool IsGenericTypeDefinition(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsGenericTypeDefinition;
#else
            return type.GetTypeInfo().IsGenericTypeDefinition;
#endif
        }

#if UNITY_WSA && !UNITY_EDITOR
        public static bool IsSubclassOf(this Type type, Type c)
        {
            return type.GetTypeInfo().IsSubclassOf(c);
        }

        public static bool IsDefined(this Type type, Type attributeType, bool inherit)
        {
            return type.GetTypeInfo().IsDefined(attributeType, inherit);
        }

        public static Type[] GetGenericParameterConstraints(this Type type)
        {
            return type.GetTypeInfo().GetGenericParameterConstraints();
        }
#endif

        public static bool IsNestedPublic(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsNestedPublic;
#else
            return type.GetTypeInfo().IsNestedPublic;
#endif        
        }

        public static bool IsPublic(this Type type)
        {
#if !UNITY_WSA || UNITY_EDITOR
            return type.IsPublic;
#else
            return type.GetTypeInfo().IsPublic;
#endif        
        }

        public static string GetFriendlyName(this Type type)
        {
            if (type == typeof(int))
                return "int";
            else if (type == typeof(short))
                return "short";
            else if (type == typeof(byte))
                return "byte";
            else if (type == typeof(bool))
                return "bool";
            else if (type == typeof(long))
                return "long";
            else if (type == typeof(float))
                return "float";
            else if (type == typeof(double))
                return "double";
            else if (type == typeof(decimal))
                return "decimal";
            else if (type == typeof(string))
                return "string";
            else if (type.IsGenericType)
                return type.FullName.Split('`')[0] + "<" + string.Join(", ", type.GetGenericArguments().Select(x => GetFriendlyName(x)).ToArray()) + ">";
            else
                return type.FullName;
        }
    }
}
