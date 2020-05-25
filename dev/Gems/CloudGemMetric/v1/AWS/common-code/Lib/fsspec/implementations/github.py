import io
import requests
from ..spec import AbstractFileSystem


class GithubFileSystem(AbstractFileSystem):
    """[Experimental] interface to files in github

    An instance of this class provides the files residing within a remote github
    repository. You may specify a point in the repos history, by SHA, branch
    or tag (default is current master).

    Given that code files tend to be small, and that github does not support
    retrieving partial content, we always fetch whole files.
    """

    url = "https://api.github.com/repos/{org}/{repo}/git/trees/{sha}"
    rurl = "https://raw.githubusercontent.com/{org}/{repo}/{sha}/{path}"
    protocol = "github"

    def __init__(self, org, repo, sha="master", **kwargs):
        super().__init__(**kwargs)
        self.org = org
        self.repo = repo
        self.root = sha
        self.ls("")

    def ls(self, path, detail=False, sha=None, **kwargs):
        if path == "":
            sha = self.root
        if sha is None:
            parts = path.rstrip("/").split("/")
            so_far = ""
            sha = self.root
            for part in parts:
                out = self.ls(so_far, True, sha=sha)
                so_far += "/" + part if so_far else part
                out = [o for o in out if o["name"] == so_far][0]
                if out["type"] == "file":
                    if detail:
                        return [out]
                    else:
                        return path
                sha = out["sha"]
        if path not in self.dircache:
            r = requests.get(self.url.format(org=self.org, repo=self.repo, sha=sha))
            self.dircache[path] = [
                {
                    "name": path + "/" + f["path"] if path else f["path"],
                    "mode": f["mode"],
                    "type": {"blob": "file", "tree": "directory"}[f["type"]],
                    "size": f.get("size", 0),
                    "sha": f["sha"],
                }
                for f in r.json()["tree"]
            ]
        if detail:
            return self.dircache[path]
        else:
            return sorted([f["name"] for f in self.dircache[path]])

    def _open(
        self,
        path,
        mode="rb",
        block_size=None,
        autocommit=True,
        cache_options=None,
        **kwargs
    ):
        if mode != "rb":
            raise NotImplementedError
        url = self.rurl.format(org=self.org, repo=self.repo, path=path, sha=self.root)
        r = requests.get(url)
        return io.BytesIO(r.content)
