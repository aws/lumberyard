# -*- coding: utf-8 -*-
"""The root of JIRA package namespace."""
from __future__ import unicode_literals
from pbr.version import VersionInfo

_v = VersionInfo('jira').semantic_version()
__version__ = _v.release_string()
version_info = _v.version_tuple()

from jira.client import Comment  # noqa: E402
from jira.client import Issue  # noqa: E402
from jira.client import JIRA  # noqa: E402
from jira.client import Priority  # noqa: E402
from jira.client import Project  # noqa: E402
from jira.client import Role  # noqa: E402
from jira.client import User  # noqa: E402
from jira.client import Watchers  # noqa: E402
from jira.client import Worklog  # noqa: E402
from jira.config import get_jira  # noqa: E402
from jira.exceptions import JIRAError  # noqa: E402

__all__ = (
    'Comment',
    '__version__',
    'Issue',
    'JIRA',
    'JIRAError',
    'Priority',
    'Project',
    'Role',
    'User',
    'version_info',
    'Watchers',
    'Worklog',
    'get_jira'
)
