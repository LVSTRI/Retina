import clang
import mako
import sys
import os

from clang.cindex import TranslationUnit, CursorKind, Cursor, Index
from mako.template import Template

KHRONOS_VENDORS = [
    '_IMG',
    '_AMD',
    '_AMDX',
    '_ARM',
    '_FSL',
    '_BRCM',
    '_NXP',
    '_NV',
    '_NVX',
    '_VIV',
    '_VSI',
    '_KDAB',
    '_ANDROID',
    '_CHROMIUM',
    '_FUCHSIA',
    '_GGP',
    '_GOOGLE',
    '_QCOM',
    '_LUNARG',
    '_NZXT',
    '_SAMSUNG',
    '_SEC',
    '_TIZEN',
    '_RENDERDOC',
    '_NN',
    '_MVK',
    '_KHR',
    '_KHX',
    '_EXT',
    '_MESA',
    '_INTEL',
    '_HUAWEI',
    '_VALVE',
    '_QNX',
    '_JUICE',
    '_FB',
    '_RASTERGRID',
    '_MSFT',
    'IMG',
    'AMD',
    'AMDX',
    'ARM',
    'FSL',
    'BRCM',
    'NXP',
    'NV',
    'NVX',
    'VIV',
    'VSI',
    'KDAB',
    'ANDROID',
    'CHROMIUM',
    'FUCHSIA',
    'GGP',
    'GOOGLE',
    'QCOM',
    'LUNARG',
    'NZXT',
    'SAMSUNG',
    'SEC',
    'TIZEN',
    'RENDERDOC',
    'NN',
    'MVK',
    'KHR',
    'KHX',
    'EXT',
    'MESA',
    'INTEL',
    'HUAWEI',
    'VALVE',
    'QNX',
    'JUICE',
    'FB',
    'RASTERGRID',
    'MSFT',
]


def has_vendor_suffix(name: str) -> bool:
    for vendor in KHRONOS_VENDORS:
        if name.endswith(vendor):
            return True
    return False


def remove_vendor_suffix(name: str) -> str:
    for vendor in KHRONOS_VENDORS:
        if name.endswith(vendor):
            return name.replace(vendor, '', 1)
    return name


def as_retina_enum_declaration(name: str, conversion: {str: str}) -> str:
    name = name.replace('Vk', '', 1)
    name = remove_vendor_suffix(name)
    if name.endswith('2'):
        name = name.replace('2', '', 1)
    if name.endswith('FlagBits'):
        name = name.replace('FlagBits', 'Flag', 1)
    if name.endswith('Op'):
        name = name.replace('Op', 'Operator', 1)
    if name in conversion:
        name = conversion[name]
    return f'E{name}'


def find_common_enum_string(name: str) -> str:
    common = ""
    stripped_name = remove_vendor_suffix(name.replace('Vk', '', 1))
    stripped_name = stripped_name.replace('FlagBits', '')
    i = 0
    while i < len(stripped_name):
        if i == 0:
            common += stripped_name[i]
            i += 1
            continue

        # Worst possible way of doing this, but it works
        if stripped_name[i].isupper() or (stripped_name[i].isdigit() and stripped_name[i - 1].islower()):
            common += '_'
        common += stripped_name[i].upper()
        i += 1
        if i == len(stripped_name):
            break
        if common[-1] == '_':
            while stripped_name[i].isdigit():
                common += stripped_name[i]
                i += 1
                if i == len(stripped_name):
                    break
    return common


def as_retina_enum_values(name: str, values: {str: int}) -> {str: int}:
    output: {str: int} = {}
    common = find_common_enum_string(name)

    for (entity, _) in values.items():
        original = entity
        entity = entity.replace('VK_', 'E_', 1)
        entity = entity.replace('_BIT', '', 1)
        entity = entity.replace(f'_{common}', '', 1)
        # entity = remove_vendor_suffix(entity)
        if 'MAX_ENUM' in entity:
            continue
        output[entity] = original
    return output


def main():
    include_directories = [
        os.path.join(os.environ['VULKAN_SDK'], 'include'),
    ]
    compile_definitions = [
        'VK_NO_PROTOTYPES'
    ]
    cpp_standard = 'c++2b'
    cpp_args = []
    for include_directory in include_directories:
        cpp_args.append(f'-I{include_directory}')
    for definition in compile_definitions:
        cpp_args.append(f'-D{definition}')

    cpp_args.append(f'-std={cpp_standard}')
    cpp_args.append(f'-x')
    cpp_args.append(f'c++')
    vulkan_header = os.path.join(include_directories[0], 'vulkan/vulkan.h')
    index = Index.create()
    translation_unit = index.parse(
        vulkan_header,
        args=cpp_args,
        options=TranslationUnit.PARSE_INCOMPLETE
    )
    for diagnostic in translation_unit.diagnostics:
        print(diagnostic)

    stack: [Cursor] = [translation_unit.cursor]
    enums: {str: {str: int}} = {}
    enums_filter = [
        # 'VkBufferUsageFlagBits',
        # 'VkPipelineCreateFlagBits',
        # 'VkFormatFeatureFlagBits',
        'VkAccessFlagBits',
        'VkPipelineStageFlagBits',
    ]
    special_enums = [
        # 'VkBufferUsageFlagBits2KHR',
        # 'VkPipelineCreateFlagBits2KHR',
        # 'VkFormatFeatureFlagBits2',
        'VkAccessFlagBits2',
        'VkPipelineStageFlagBits2',
        'VkPhysicalDeviceSchedulingControlsFlagBitsARM',
        'VkMemoryDecompressionMethodFlagBitsNV',
    ]

    while len(stack) > 0:
        current = stack.pop()
        if current.location.file is not None and include_directories[0] in current.location.file.name:
            if current.kind == CursorKind.ENUM_DECL:
                if current.spelling in enums_filter:
                    continue
                enums[current.spelling] = {}
                for child in current.get_children():
                    enums[current.spelling][child.spelling] = int(child.enum_value)
                continue
            if current.kind == CursorKind.VAR_DECL:
                typename: str = current.type.spelling
                if current.type.is_const_qualified():
                    typename = typename.replace('const ', '', 1)
                if typename not in enums:
                    enums[typename] = {}
                enums[typename][current.spelling] = -1
        for child in current.get_children():
            stack.append(child)

    duplicate_keys: [str] = []
    for (k, v) in enums.items():
        if has_vendor_suffix(k):
            removed_vendor = remove_vendor_suffix(k)
            if removed_vendor in enums:
                duplicate_keys.append(k)

    for k in duplicate_keys:
        del enums[k]

    '''
    duplicate_values: {str: [str]} = {}
    for (k, v) in enums.items():
        if k not in duplicate_values:
            duplicate_values[k] = []
        for (u, t) in v.items():
            if has_vendor_suffix(u):
                removed_vendor = remove_vendor_suffix(u)
                if removed_vendor in v:
                    duplicate_values[k].append(u)

    for (k, v) in duplicate_values.items():
        for x in v:
            del enums[k][x]
    '''

    conversion_enum_filter = [
        'VkAcquireProfilingLockFlagBitsKHR',
        'VkPipelineCompilerControlFlagBitsAMD',
        'VkShaderCorePropertiesFlagBitsAMD',
    ]
    converted_enums: {str: {str: str}} = {}
    converted_enum_mapping: {str: str} = {}
    for (k, v) in enums.items():
        if k.startswith('Std') or k in conversion_enum_filter:
            continue
        converted_declaration_name = as_retina_enum_declaration(k, {
            'Format': 'ResourceFormat',
            'AccessFlag': 'ResourceAccessFlag',
            'DescriptorSetLayoutCreateFlag': 'DescriptorLayoutCreateFlag',
        })
        converted_enums[converted_declaration_name] = as_retina_enum_values(k, v)
        converted_enum_mapping[k] = converted_declaration_name
        converted_enum_mapping[converted_declaration_name] = k

    template = Template(

"""#pragma once

/* This file was generated automatically, do not edit directly. */

#include <Retina/Core/Core.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <type_traits>

namespace Retina::Graphics {
% for (name, entries) in converted_enums.items():
<%
underlying_type = ''
native_name = converted_enum_mapping[name]
if native_name in special_enums:
    underlying_type = 'uint64'
else:
    underlying_type = f'std::underlying_type_t<{native_name}>';
%>  // ${native_name}
  enum class ${name} : ${underlying_type} {
  % for (key, value) in entries.items():
    ${key} = ${value},
  % endfor
  };

% endfor
  namespace Details {
    template <typename T>
    struct SEnumCounterpart;

    #define RETINA_ENUM_COUNTERPART_SPECIALIZATION(E, T)           ${backslash}
      template <> struct SEnumCounterpart<E> { using Type = T; };  ${backslash}
      template <> struct SEnumCounterpart<T> { using Type = E; }

  % for (name, entries) in converted_enums.items():
  % if not converted_enum_mapping[name] in special_enums:
    RETINA_ENUM_COUNTERPART_SPECIALIZATION(${converted_enum_mapping[name]}, ${name});
  % endif
  % endfor

    #undef RETINA_ENUM_COUNTERPART_SPECIALIZATION

    template <typename T>
    using EnumCounterpartType = typename SEnumCounterpart<T>::Type;
  }

  template <typename E>
  constexpr auto AsEnumCounterpart(E e) noexcept -> Details::EnumCounterpartType<E> {
    return static_cast<Details::EnumCounterpartType<E>>(e);
  }

  % for special in special_enums:
  constexpr auto AsEnumCounterpart(${converted_enum_mapping[special]} e) noexcept -> ${special} {
    return static_cast<${special}>(e);
  }
  % endfor

  #define RETINA_ENUM_AS_STRING_OVERLOAD(E, T)             ${backslash}
    constexpr auto ToString(T e) noexcept -> std::string { ${backslash}
      return RETINA_CONCAT(string_, E)(e);                 ${backslash}
    }

  #define RETINA_ENUM_AS_STRING_CONVERSION_OVERLOAD(E, T)     ${backslash}
    constexpr auto ToString(T e) noexcept -> std::string {    ${backslash}
      return RETINA_CONCAT(string_, E)(AsEnumCounterpart(e)); ${backslash}
    }

  #define RETINA_ENUM_AS_STRING_BASE_AND_CONVERSION_OVERLOAD(E, T) ${backslash}
    constexpr auto ToString(T e) noexcept -> std::string {         ${backslash}
      return RETINA_CONCAT(string_, E)(AsEnumCounterpart(e));      ${backslash}
    }                                                              ${backslash}
                                                                   ${backslash}
    constexpr auto ToString(E e) noexcept -> std::string {         ${backslash}
      return RETINA_CONCAT(string_, E)(e);                         ${backslash}
    }

  #define RETINA_FLAG_ENUM_AS_STRING_OVERLOAD(F, T, E) ${backslash}
    RETINA_ENUM_AS_STRING_CONVERSION_OVERLOAD(E, T)    ${backslash}
    RETINA_ENUM_AS_STRING_OVERLOAD(E, F)

% for (name, entries) in converted_enums.items():
  % if not converted_enum_mapping[name] in special_enums:
  % if name.endswith('Flag'):
  RETINA_FLAG_ENUM_AS_STRING_OVERLOAD(${converted_enum_mapping[name]}, ${name}, ${converted_enum_mapping[name].replace('FlagBits', 'Flags', 1)})
  % else:
  RETINA_ENUM_AS_STRING_BASE_AND_CONVERSION_OVERLOAD(${converted_enum_mapping[name]}, ${name})
  % endif
  % endif
% endfor

% for special in special_enums:
  RETINA_ENUM_AS_STRING_CONVERSION_OVERLOAD(${special}, ${converted_enum_mapping[special]})
% endfor

  #undef RETINA_ENUM_AS_STRING_OVERLOAD
  #undef RETINA_ENUM_AS_STRING_CONVERSION_OVERLOAD
  #undef RETINA_ENUM_AS_STRING_BASE_AND_CONVERSION_OVERLOAD
  #undef RETINA_FLAG_ENUM_AS_STRING_OVERLOAD
 
  // Useful constants 
  constexpr auto EXTERNAL_SUBPASS = VK_SUBPASS_EXTERNAL;
  constexpr auto LOD_CLAMP_NONE = VK_LOD_CLAMP_NONE;
  constexpr auto SUBRESOURCE_LEVEL_IGNORED = -1_u32;
  constexpr auto SUBRESOURCE_LAYER_IGNORED = -1_u32;
  constexpr auto SUBRESOURCE_REMAINING_LEVELS = VK_REMAINING_MIP_LEVELS;
  constexpr auto SUBRESOURCE_REMAINING_LAYERS = VK_REMAINING_ARRAY_LAYERS;
  constexpr auto WHOLE_SIZE = VK_WHOLE_SIZE;
  constexpr auto ATTACHMENT_UNUSED = VK_ATTACHMENT_UNUSED;
  constexpr auto QUEUE_FAMILY_IGNORED = VK_QUEUE_FAMILY_IGNORED;

  template <typename T, typename U = std::underlying_type_t<T>>
    requires (std::is_scoped_enum_v<T>)
  RETINA_NODISCARD constexpr auto operator &(T left, T right) noexcept -> T {
    return static_cast<T>(static_cast<U>(left) & static_cast<U>(right));
  }

  template <typename T, typename U = std::underlying_type_t<T>>
    requires (std::is_scoped_enum_v<T>)
  RETINA_NODISCARD constexpr auto operator |(T left, T right) noexcept -> T {
    return static_cast<T>(static_cast<U>(left) | static_cast<U>(right));
  }

  template <typename T, typename U = std::underlying_type_t<T>>
    requires (std::is_scoped_enum_v<T>)
  RETINA_NODISCARD constexpr auto operator ^(T left, T right) noexcept -> T {
    return static_cast<T>(static_cast<U>(left) ^ static_cast<U>(right));
  }

  template <typename T>
    requires (std::is_scoped_enum_v<T>)
  constexpr auto operator &=(T& left, T right) noexcept -> T& {
    return left = left & right;
  }

  template <typename T>
    requires (std::is_scoped_enum_v<T>)
  constexpr auto operator |=(T& left, T right) noexcept -> T& {
    return left = left | right;
  }

  template <typename T>
    requires (std::is_scoped_enum_v<T>)
  constexpr auto operator ^=(T& left, T right) noexcept -> T& {
    return left = left ^ right;
  }

  template <typename T, typename U = std::underlying_type_t<T>>
    requires (std::is_scoped_enum_v<T>)
  RETINA_NODISCARD constexpr auto operator ~(T value) noexcept -> T {
    return static_cast<T>(~static_cast<U>(value));
  }
}
"""
    )

    output = template.render(**{
        'converted_enums': dict(reversed(converted_enums.items())),
        'converted_enum_mapping': converted_enum_mapping,
        'special_enums': special_enums,
        'backslash': '\\'
    })
    # write to file
    with open('Enums.hpp', 'w') as f:
        f.write(output)

    return


main()
