
package ${ANDROID_PACKAGE};

import android.util.Log;
import com.amazon.lumberyard.LumberyardActivity;

////////////////////////////////////////////////////////////////////////////////////////////////////
public class ${ANDROID_PROJECT_ACTIVITY} extends LumberyardActivity
{
    // load the required shared libraries for startup
    static
    {
        Log.d("LMBR", "BootStrap: Starting Library load");
        System.loadLibrary("c++_shared");
        System.loadLibrary("${ANDROID_LAUNCHER_NAME}");
        Log.d("LMBR", "BootStrap: Finished Library load");
    }
}