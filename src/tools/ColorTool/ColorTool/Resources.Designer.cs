//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.42000
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace ColorTool {
    using System;
    
    
    /// <summary>
    ///   A strongly-typed resource class, for looking up localized strings, etc.
    /// </summary>
    // This class was auto-generated by the StronglyTypedResourceBuilder
    // class via a tool like ResGen or Visual Studio.
    // To add or remove a member, edit your .ResX file then rerun ResGen
    // with the /str option, or rebuild your VS project.
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("System.Resources.Tools.StronglyTypedResourceBuilder", "15.0.0.0")]
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    public class Resources {
        
        private static global::System.Resources.ResourceManager resourceMan;
        
        private static global::System.Globalization.CultureInfo resourceCulture;
        
        [global::System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        internal Resources() {
        }
        
        /// <summary>
        ///   Returns the cached ResourceManager instance used by this class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        public static global::System.Resources.ResourceManager ResourceManager {
            get {
                if (object.ReferenceEquals(resourceMan, null)) {
                    global::System.Resources.ResourceManager temp = new global::System.Resources.ResourceManager("ColorTool.Resources", typeof(Resources).Assembly);
                    resourceMan = temp;
                }
                return resourceMan;
            }
        }
        
        /// <summary>
        ///   Overrides the current thread's CurrentUICulture property for all
        ///   resource lookups using this strongly typed resource class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        public static global::System.Globalization.CultureInfo Culture {
            get {
                return resourceCulture;
            }
            set {
                resourceCulture = value;
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Error loading ini file &quot;{0}&quot;.
        /// </summary>
        public static string IniLoadError {
            get {
                return ResourceManager.GetString("IniLoadError", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Error loading ini file &quot;{0}&quot;
        ///    for key &quot;{1}&quot;
        ///    the value &quot;{2}&quot; is invalid.
        /// </summary>
        public static string IniParseError {
            get {
                return ResourceManager.GetString("IniParseError", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Invalid Color.
        /// </summary>
        public static string InvalidColor {
            get {
                return ResourceManager.GetString("InvalidColor", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Invalid scheme - did not find 16 colors.
        /// </summary>
        public static string InvalidNumberOfColors {
            get {
                return ResourceManager.GetString("InvalidNumberOfColors", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Usage: colortool -o &lt;filename&gt;.
        /// </summary>
        public static string OutputUsage {
            get {
                return ResourceManager.GetString("OutputUsage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Could not find or load &quot;{0}&quot;.
        /// </summary>
        public static string SchemeNotFound {
            get {
                return ResourceManager.GetString("SchemeNotFound", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Usage:
        ///    colortool.exe [options] &lt;schemename&gt;
        ///ColorTool is a utility for helping to set the color palette of the Windows Console.
        ///By default, applies the colors in the specified .itermcolors or .ini file to the current console window.
        ///This does NOT save the properties automatically. For that, you&apos;ll need to open the properties sheet and hit &quot;Ok&quot;.
        ///Included should be a `schemes/` directory with a selection of schemes of both formats for examples.
        ///Feel free to add your own preferred scheme to that directory. [rest of string was truncated]&quot;;.
        /// </summary>
        public static string Usage {
            get {
                return ResourceManager.GetString("Usage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Wrote selected scheme to the defaults..
        /// </summary>
        public static string WroteToDefaults {
            get {
                return ResourceManager.GetString("WroteToDefaults", resourceCulture);
            }
        }
    }
}
