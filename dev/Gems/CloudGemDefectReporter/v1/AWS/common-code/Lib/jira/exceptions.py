import os
import tempfile


class JIRAError(Exception):
    """General error raised for all problems in operation of the client."""

    log_to_tempfile = True
    if 'TRAVIS' in os.environ:
        log_to_tempfile = False  # Travis is keeping only the console log.

    def __init__(self, status_code=None, text=None, url=None, request=None, response=None, **kwargs):
        self.status_code = status_code
        self.text = text
        self.url = url
        self.request = request
        self.response = response
        self.headers = kwargs.get('headers', None)
        self.log_to_tempfile = False
        self.travis = False
        if 'PYJIRA_LOG_TO_TEMPFILE' in os.environ:
            self.log_to_tempfile = True
        if 'TRAVIS' in os.environ:
            self.travis = True

    def __str__(self):
        """Return a string representation of the error."""
        t = "JiraError HTTP %s" % self.status_code
        if self.url:
            t += " url: %s" % self.url

        details = ""
        if self.request is not None and hasattr(self.request, 'headers'):
            details += "\n\trequest headers = %s" % self.request.headers

        if self.request is not None and hasattr(self.request, 'text'):
            details += "\n\trequest text = %s" % self.request.text

        if self.response is not None and hasattr(self.response, 'headers'):
            details += "\n\tresponse headers = %s" % self.response.headers

        if self.response is not None and hasattr(self.response, 'text'):
            details += "\n\tresponse text = %s" % self.response.text

        # separate logging for Travis makes sense.
        if self.travis:
            if self.text:
                t += "\n\ttext: %s" % self.text
            t += details
        # Only log to tempfile if the option is set.
        elif self.log_to_tempfile:
            fd, file_name = tempfile.mkstemp(suffix='.tmp', prefix='jiraerror-')
            with open(file_name, "w") as f:
                t += " details: %s" % file_name
                f.write(details)
        # Otherwise, just return the error as usual
        else:
            if self.text:
                t += "\n\ttext: %s" % self.text
            t += "\n\t" + details

        return t
