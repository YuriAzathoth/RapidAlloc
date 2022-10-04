#
# RapidAllocator - Fast C allocators collections
# Copyright (C) 2022 Yuriy Zinchenko
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

CMAKE_MINIMUM_REQUIRED (VERSION 2.8.12)

IF (COMMAND CMAKE_POLICY)
	CMAKE_POLICY (SET CMP0048 NEW)
	CMAKE_POLICY (SET CMP0079 NEW)
ENDIF ()

PROJECT (RapidAlloc VERSION 0.0.0.0)

SET (CMAKE_C_STANDARD 11)
SET (CMAKE_C_STANDARD_REQUIRED ON)

SET (CMAKE_EXPORT_COMPILE_COMMANDS ON)
IF (CMAKE_EXTRA_GENERATOR)
	IF ("${CMAKE_EXTRA_GENERATOR}" STREQUAL "CodeBlocks")
		SET (CMAKE_CODEBLOCKS_EXCLUDE_EXTERNAL_FILES ON)
	ELSEIF ("${CMAKE_EXTRA_GENERATOR}" STREQUAL "CodeLite")
		SET (CMAKE_CODELITE_USE_TARGETS ON)
	ENDIF ()
	IF (NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
		EXECUTE_PROCESS (COMMAND
			${CMAKE_COMMAND} -E create_symlink
			${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
			${CMAKE_SOURCE_DIR}/compile_commands.json
		)
	ENDIF ()
ENDIF ()

INCLUDE (CTest REQUIRED)
ENABLE_TESTING ()

FILE (GLOB SOURCE_FILES *.c)
FILE (GLOB HEADER_FILES *.h)
ADD_EXECUTABLE (Test ${SOURCE_FILES} ${HEADER_FILES})
