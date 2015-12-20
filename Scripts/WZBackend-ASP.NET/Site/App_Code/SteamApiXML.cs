using System;
using System.Diagnostics;
using System.Xml;
using System.Xml.Serialization;

namespace SteamXML
{
    [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
    [System.SerializableAttribute()]
    [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
    public class r_MicroTxnParams
    {
        [System.Xml.Serialization.XmlElementAttribute(Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public string orderid;
        [System.Xml.Serialization.XmlElementAttribute(Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public string transid;
        [System.Xml.Serialization.XmlElementAttribute(Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public string status;

        // from GetUserInfo
        [System.Xml.Serialization.XmlElementAttribute(Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public string currency;
        [System.Xml.Serialization.XmlElementAttribute(Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public string country;
    }

    [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
    [System.SerializableAttribute()]
    [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
    public class r_error
    {
        [System.Xml.Serialization.XmlElementAttribute(Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public int errorcode;
        [System.Xml.Serialization.XmlElementAttribute(Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public string errordesc;
    }

    /// <remarks/>
    [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
    [System.SerializableAttribute()]
    [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
    [System.Xml.Serialization.XmlRootAttribute("response", Namespace = "", IsNullable = false)]
    public class MicroTnxResponse
    {
        [System.Xml.Serialization.XmlElementAttribute("result", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public string result;
        [System.Xml.Serialization.XmlElementAttribute("params", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public r_MicroTxnParams params_;
        [System.Xml.Serialization.XmlElementAttribute("error", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public r_error error;
    }

    [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
    [System.SerializableAttribute()]
    [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
    public class r_AuthParams
    {
        [System.Xml.Serialization.XmlElementAttribute(Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public string result;
        [System.Xml.Serialization.XmlElementAttribute(Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public UInt64 steamid;
    }

    /// <remarks/>
    [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
    [System.SerializableAttribute()]
    [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
    [System.Xml.Serialization.XmlRootAttribute("response", Namespace = "", IsNullable = false)]
    public class AuthResponse
    {
        [System.Xml.Serialization.XmlElementAttribute("params", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public r_AuthParams params_;
        [System.Xml.Serialization.XmlElementAttribute("error", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public r_error error;
    }

    /// <remarks/>
    [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
    [System.SerializableAttribute()]
    [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
    [System.Xml.Serialization.XmlRootAttribute("appownership", Namespace = "", IsNullable = false)]
    public class CheckAppOwnershipResponse
    {
        [System.Xml.Serialization.XmlElementAttribute("ownsapp", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public bool ownsapp;
        [System.Xml.Serialization.XmlElementAttribute("permanent", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public bool permanent;
    }

    /// <remarks/>
    [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.3038")]
    [System.SerializableAttribute()]
    [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
    [System.Xml.Serialization.XmlRootAttribute("appownership", Namespace = "", IsNullable = false)]
    public partial class GetPublisherAppOwnershipResponse
    {
        /// <remarks/>
        [System.Xml.Serialization.XmlElementAttribute("apps", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public appownershipApps apps;
    }

    /// <remarks/>
    [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.3038")]
    [System.SerializableAttribute()]
    [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
    public partial class appownershipApps
    {
        private appownershipAppsApp[] appField;

        /// <remarks/>
        [System.Xml.Serialization.XmlElementAttribute("app", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public appownershipAppsApp[] app
        {
            get
            {
                return this.appField;
            }
            set
            {
                this.appField = value;
            }
        }
    }

    /// <remarks/>
    [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.3038")]
    [System.SerializableAttribute()]
    [System.Diagnostics.DebuggerStepThroughAttribute()]
    [System.ComponentModel.DesignerCategoryAttribute("code")]
    [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
    public partial class appownershipAppsApp
    {
        [System.Xml.Serialization.XmlElementAttribute("appid", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public int appid;
        [System.Xml.Serialization.XmlElementAttribute("ownsapp", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public bool ownsapp;
        [System.Xml.Serialization.XmlElementAttribute("permanent", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public bool permanent;
    }
};
