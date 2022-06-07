"""
Defining handler logic for the C rows.
"""

from __future__ import annotations

import re
import subprocess
from fnmatch import fnmatch
from pathlib import Path
from typing import TYPE_CHECKING

from .row_handler_common import CORE_DIR, CommonRow

if TYPE_CHECKING:  # pragma: no cover
    from .api import DataRow
    from .source_definition_cache import SourceDefinitionCache


class CRow(CommonRow):
    language = "C"

    def __init__(self, source_def_cache: SourceDefinitionCache | None = None) -> None:
        super().__init__(source_def_cache)

    def _get_module_and_function(self, symbol_name: str) -> tuple[str, str]:
        # There is no other info than the symbol name itself
        # Just cleaning some names that are known to have number suffixes in them
        return "", self._clean_special_symbols(symbol_name)

    @staticmethod
    def _clean_special_symbols(symbol_name: str) -> str:
        if re.match(r"^OUTLINED_FUNCTION_\d+$", symbol_name):
            return "OUTLINED_FUNCTION"
        if re.match(r"^__compound_literal\.\d+$", symbol_name):
            return "__compound_literal"
        if re.match(r"^__func__\.\d+$", symbol_name):
            return "__func__"

        return symbol_name

    def _get_definition(self, row: DataRow) -> str:
        symbol_name = row.func_name

        # e.g. [section .flash], [section .flash2]
        if symbol_name.startswith("[section"):
            return ""

        # We might not find the definition, so have at least some idea of where it lives
        really_existing_definition = self._get_existing_definition(symbol_name)
        if really_existing_definition:
            result = really_existing_definition
        else:
            result = self._get_default_definition(symbol_name)

        # Sanitating the path to be relative to the core dir
        if "trezor-firmware/core/" in result:
            result = result.split("trezor-firmware/core/", maxsplit=1)[1]
        return result

    def _get_existing_definition(
        self, symbol_name: str, accept_declaration: bool = False
    ) -> str:

        # e.g. groestl_big_close.constprop.0 -> groestl_big_close
        if "." in symbol_name:
            symbol_name = symbol_name.split(".")[0]

        # First we grep all the possible occurrences of the symbol name
        # and only then process them more further
        to_search = fr"\b{symbol_name}\b"

        if symbol_name.startswith("mod_trezor"):
            where_to_search = CORE_DIR / "embed" / "extmod"
        else:
            where_to_search = f'{CORE_DIR / "vendor"} {CORE_DIR / "embed"}'

        # Important to use "-R" instead of "-r", as vendor contains only symlinks
        cmd = fr'grep -R -P -n0 --include \*.h --include \*.c "{to_search}" {where_to_search}'

        grep_result = subprocess.run(
            cmd, stdout=subprocess.PIPE, text=True, shell=True
        ).stdout.strip()

        if not grep_result or len(grep_result.split(":")) < 3:
            return ""
        else:
            grep_lines = [
                line for line in grep_result.split("\n") if symbol_name in line
            ]
            return self._find_definition_in_lines(
                symbol_name, grep_lines, accept_declaration
            )

    def _find_definition_in_lines(
        self, symbol_name: str, grep_lines: list[str], accept_declaration: bool = False
    ) -> str:
        for grep_line in grep_lines:
            file_name, line_num, line_content = grep_line.split(":", maxsplit=2)
            if self._validate_line_for_definition(
                line_content, symbol_name, accept_declaration
            ):
                return f"{file_name}:{line_num}"

        return ""

    @staticmethod
    def _validate_line_for_definition(
        line_content: str, symbol_name: str, accept_declaration: bool = False
    ) -> bool:
        # Loosening the rules when we are OK just with declaration
        # TODO: it is dirty with `accept_declaration`, but this way
        # we can reuse otherwise same logic for both definition and declaration

        line_content = line_content.strip()
        after_symbols = line_content.split(symbol_name, maxsplit=1)[-1]

        if symbol_name.startswith("Font_Roboto"):
            return "const" in line_content

        if not accept_declaration:
            if line_content.endswith(";") and "=" not in after_symbols:
                return False

        if line_content.startswith(("/", "*", "#")):
            return False

        # Definition is above the line - e.g. groestl_big_core
        if line_content.startswith(f"{symbol_name}("):
            return True
        # Definition is above the line - e.g. words_button_seq
        if line_content.startswith(f"}} {symbol_name}"):
            return True

        if not accept_declaration:
            if not after_symbols.startswith(("[", "(", " =")) and not re.match(
                r"^ *?\(", after_symbols
            ):
                return False

        before_symbols = line_content.split(symbol_name, maxsplit=1)[0]
        if "," in before_symbols:
            return False
        if "ALIGN" not in before_symbols and "(" in before_symbols:
            return False

        before_symbols_words = before_symbols.split()

        def_pattern_words = [
            "static",
            "const",
            "void",
            "bool",
            "char",
            "char*",  # how to escape the asterisk? it should be literal
            "secbool",
            "float",
            "uint*",
            "int*",
            "__int*",
            "mp_obj_t",
            "mp_*",
            "size_t",
            "qstr",
            "STATIC",
            "FRESULT",
            "DRESULT",
            "DWORD",
            "*TypeDef",
            "*RETURN",
        ]
        if "_context" in symbol_name:
            def_pattern_words.append("secp256k1_context")

        for word in before_symbols_words:
            for def_pattern in def_pattern_words:
                if fnmatch(word, def_pattern):
                    return True

        return False

    def _get_default_definition(self, symbol_name: str) -> str:
        # When definition was not found, look at least for a declaration
        # In this case, not specifying the exact line (to also differentiate it)
        possible_declaration = self._get_existing_definition(
            symbol_name, accept_declaration=True
        )
        if possible_declaration:
            # Stripping the line number
            return possible_declaration.split(":")[0]

        # These things are special, defined in the embed/extmod
        if symbol_name.startswith(("mod_", "mp_")) and symbol_name.endswith(
            ("_obj", "_locals_dict", "_locals_dict_table", "_globals")
        ):
            mod_mp_result = self._find_at_least_something(
                symbol_name, CORE_DIR / "embed" / "extmod"
            )
            if mod_mp_result:
                return mod_mp_result

        vendor = self._find_at_least_something(symbol_name, CORE_DIR / "vendor")
        if vendor:
            return vendor

        return self._find_at_least_something(symbol_name, CORE_DIR / "embed")

    def _find_at_least_something(
        self, symbol_name: str, dir_to_search: str | Path
    ) -> str:
        cmd = fr'grep -R -P -l --include \*.h --include \*.c "\b{symbol_name}" {dir_to_search}'

        grep_result = subprocess.run(
            cmd, stdout=subprocess.PIPE, text=True, shell=True
        ).stdout.strip()

        if not grep_result:
            return ""
        else:
            return grep_result.split("\n")[0]
