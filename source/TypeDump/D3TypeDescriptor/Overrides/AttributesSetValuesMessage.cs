﻿﻿/*
 * Copyright (C) 2011 mooege project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

 using System;
 using System.Text;

namespace D3TypeDescriptor.Overrides
{
    class AttributesSetValuesMessage : GameMessageDescriptor
    {
        public override void LoadFields(FieldDescriptor[] fields)
        {
            base.LoadFields(fields); // removes RequiredMessageHeader
            if (Fields.Length != 3 ||
                Fields[0].Type.Name != "DT_INT" ||
                Fields[1].Type.Name != "DT_FIXEDARRAY" || Fields[1].SubType.Name != "NetAttributeKeyValue" ||
                Fields[2].Type != null)
                throw new Exception("Unexpected fields in AttributesSetValuesMessage.");
        }

        public override void GenerateParseBody(StringBuilder b, int pad, string bitBufferName)
        {
            base.GenerateParseBody(b, pad, bitBufferName);
            var fieldName = Fields[1].GetFieldName();
            b.IndentAppendLine(pad, "for (int i = 0; i < " + fieldName + ".Length; i++) { " + fieldName + "[i].ParseValue(" + bitBufferName + "); };");
        }

        public override void GenerateEncodeBody(StringBuilder b, int pad, string bitBufferName)
        {
            base.GenerateEncodeBody(b, pad, bitBufferName);
            var fieldName = Fields[1].GetFieldName();
            b.IndentAppendLine(pad, "for (int i = 0; i < " + fieldName + ".Length; i++) { " + fieldName + "[i].EncodeValue(" + bitBufferName + "); };");
        }
    }
}
